/**
 * 	cpu_info:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class …
 */

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include "cpu_info.h"
#include <format>

#ifdef linux
#include <sched.h>
#include <sys/sysinfo.h>
#endif

cpu_info::cpu_info()
{
	// For initialization, setup globals
	CPUID_REGISTERS CpuidRegisters = ReadCpuid(0, 0);
	if (CpuidRegisters.x.Register.Eax >= 0xB)
	{
		sourceLeaf = CpuidRegisters.x.Register.Eax >= 0x1F ? 0x1F : 0x0B;
		if (ReadCpuid(sourceLeaf, 0).x.Register.Ebx != 0)
			if (sourceLeaf >= 0x1F)
				ParseCpu_CpuidManyDomain();
			else
				ParseCpu_CpuidThreeDomain();
	}
	else
		sourceLeaf = 1, ParseCpu_CpuidLegacy();
}

int cpu_info::countLevel(cpu_domain lvl) const noexcept
{
	if (lvl == cpu_domain::InvalidDomain || lvl_ids.size() < (size_t)lvl)
		return 1;
	return lvl_ids[lvl - 1].size();
}

CPUID_REGISTERS cpu_info::ReadCpuid(unsigned int Leaf, unsigned int Subleaf)
{
	CPUID_REGISTERS CpuidRegisters{};
#if _WIN32
	__cpuidex(reinterpret_cast<int *>(&CpuidRegisters.x.Registers[0]), Leaf, Subleaf);

#elif linux
	unsigned int ReturnEax;
	unsigned int ReturnEbx;
	unsigned int ReturnEcx;
	unsigned int ReturnEdx;

	asm("movl %4, %%eax\n"
		"movl %5, %%ecx\n"
		"CPUID\n"
		"movl %%eax, %0\n"
		"movl %%ebx, %1\n"
		"movl %%ecx, %2\n"
		"movl %%edx, %3\n"
		: "=r"(ReturnEax), "=r"(ReturnEbx), "=r"(ReturnEcx), "=r"(ReturnEdx)
		: "r"(Leaf), "r"(Subleaf)
		: "%eax", "%ebx", "%ecx", "%edx");

	CpuidRegisters.x.Register.Eax = ReturnEax;
	CpuidRegisters.x.Register.Ebx = ReturnEbx;
	CpuidRegisters.x.Register.Ecx = ReturnEcx;
	CpuidRegisters.x.Register.Edx = ReturnEdx;
#else
#endif
	return CpuidRegisters;
}

void cpu_info::ParseCpu_CpuidLegacy(void)
{
	unsigned int MaximumAddressibleIdsPhysicalPackage{1};
	unsigned int MaximumAddressibleIdsCores{};
	unsigned int LogicalProcessorsPerCore{1};
	unsigned int LogicalProcessorsPerPackage{1};
	unsigned int PackageShift{};
	unsigned int LogicalProcessorShift{};
	CPUID_REGISTERS CpuidRegisters{ReadCpuid(1, 0)};
	/*  MaximumAddressibleIdsPhysicalPackage
	 *
	 *      CPUID.1.EBX[23:16]
	 *      Maximum number of addressable IDs for logical processors in this physical package
	 *
	 *  This is the legacy value for determining the package mask and has been superceded by Leaf 0Bh and Leaf 01Fh.
	 *  Since this is a byte, processors are already exceeding 256 addressible IDs either due to topology domains or
	 *  simply having more processors in a package.
	 *
	 *      CPUID.1.EDX[28].HTT
	 *      The Maximum number of addressable IDs for logical processor in this package is valid when set to 1.
	 */

	// Determine that CPUID.1.EDX[28].HTT == 1, if this is not set it would be a very old platform.
	if (CpuidRegisters.x.Register.Edx & ((unsigned int)1 << 28))
	{
		MaximumAddressibleIdsPhysicalPackage = (unsigned int)((CpuidRegisters.x.Register.Ebx >> 16) & 0xFF);
		// This would be a 20+ year old platform to not support CPUID.4
		if ((CpuidRegisters = ReadCpuid(0, 0)).x.Register.Eax >= 4)
		{
			/* MaximumAddressibleIdsCores
			 *
			 *      CPUID.4.0.EAX[31:26]
			 *       Maximum number of addressable IDs for processor cores in the physical Package
			 *
			 *  This is the legacy value for determining the core/SMT mask and has been superceded by Leaf 0Bh and Leaf 01Fh.
			 *  Since this is 6 bits, processors are already exceeding this value addressible IDs either due to topology domains or
			 *  simply having more processors in a package.
			 */
			MaximumAddressibleIdsCores = (unsigned int)(ReadCpuid(4, 0).x.Register.Eax >> 26) + 1;
			// Determine the number of LogicalProcessors per core.
			LogicalProcessorsPerCore = MaximumAddressibleIdsPhysicalPackage / MaximumAddressibleIdsCores;
			LogicalProcessorsPerPackage = MaximumAddressibleIdsPhysicalPackage;
			LogicalProcessorShift = Tools_CreateTopologyShift(LogicalProcessorsPerCore);
			PackageShift = Tools_CreateTopologyShift(LogicalProcessorsPerPackage);
		}
		else
		{ // You cannot report Cores here, a Package == Core and so this only reports SMT within a Package.
			LogicalProcessorsPerCore = MaximumAddressibleIdsPhysicalPackage;
			LogicalProcessorsPerPackage = MaximumAddressibleIdsPhysicalPackage;

			LogicalProcessorShift = PackageShift = Tools_CreateTopologyShift(MaximumAddressibleIdsPhysicalPackage);
		}
	}
	else // You do not report Cores or SMT here.  It's always 1 Logical Processor.
		LogicalProcessorShift = PackageShift = Tools_CreateTopologyShift(MaximumAddressibleIdsPhysicalPackage);

	ThreeDomainFinalize(PackageShift, LogicalProcessorShift);
}

void cpu_info::ParseCpu_CpuidThreeDomain()
{
	unsigned int LogicalProcessorShift = 0;
	unsigned int PackageShift = 0;
	unsigned int DomainType;
	unsigned int DomainShift;
	unsigned int Subleaf = 0;
	CPUID_REGISTERS CpuidRegisters = ReadCpuid(sourceLeaf, Subleaf);

	while (CpuidRegisters.x.Register.Ebx != 0)
	{
		// CPUID.B or 1F.x.ECX[15:8] = Level Type / Domain Type
		DomainType = (CpuidRegisters.x.Register.Ecx >> 8) & 0xFF;

		// CPUID.B or 1F.x.EAX[4:0] = Level Shift / Domain Shift
		DomainShift = CpuidRegisters.x.Register.Eax & 0x1F;

		// Determine if we have enumerated Logical Processor Domain, this will
		// always be CPUID.x.0 so can also do an ordered verification.
		//
		// Also determine that nothing has an invalid Domain Type.
		switch (DomainType)
		{
		case InvalidDomain:
			/* Optionally log an error */
			break;

		case LogicalDomain:
			LogicalProcessorShift = DomainShift;
			break;
		}
		/*
		 * In three Domain topology, we do not care what the last Domain is.  Whatever
		 * it is this is the mask for the package and it is also the mask for the core
		 * since we are only recognizing three Domains.  It is always the core relationship
		 * to the package.
		 *
		 * It is incorrect to check for core id (2) because then if there was another Domain
		 * above core id, you would then mistake it as the package identifier.
		 */
		PackageShift = DomainShift;
		CpuidRegisters = ReadCpuid(sourceLeaf, ++Subleaf);
	}

	ThreeDomainFinalize(PackageShift, LogicalProcessorShift);
}

void cpu_info::ParseCpu_CpuidManyDomain()
{
	unsigned int Subleaf{0};
	unsigned int DomainType{};
	unsigned int DomainShift{};
	CPUID_REGISTERS CpuidRegisters{ReadCpuid(sourceLeaf, Subleaf)};
	APICID_BIT_LAYOUT_CTX ApicidBitLayoutCtx{.NumberOfApicIdBits = 32};

	while (CpuidRegisters.x.Register.Ebx != 0)
	{
		// CPUID.B or 1F.x.ECX[15:8] = Level Type / Domain Type
		DomainType = (CpuidRegisters.x.Register.Ecx >> 8) & 0xFF;
		// CPUID.B or 1F.x.EAX[4:0] = Level Shift / Domain Shift
		DomainShift = CpuidRegisters.x.Register.Eax & 0x1F;
		/*
		 * Best to check for known domains explicity since the ones you use
		 * may not be in sequential ordering.
		 */
		switch (DomainType)
		{
		case InvalidDomain:
			/*  This would be an error, could log it. */
		case LogicalDomain:
		case CoreDomain:
		case ModuleDomain:
		case TileDomain:
		case DieDomain:
		case DieGrpDomain:
			ApicidBitLayoutCtx.ShiftValues[ApicidBitLayoutCtx.PackageDomainIndex] = DomainShift;
			ApicidBitLayoutCtx.ShiftValueDomain[ApicidBitLayoutCtx.PackageDomainIndex] = DomainType;
			ApicidBitLayoutCtx.PackageDomainIndex++;
			break;

		default:
			// First Domain is always Logical Processor, so we will always have a valid previous.
			ApicidBitLayoutCtx.ShiftValues[ApicidBitLayoutCtx.PackageDomainIndex - 1] = DomainShift;
		}
		CpuidRegisters = ReadCpuid(sourceLeaf, ++Subleaf);
	}
	ParseCpu_CreateDomainMaskMatrix(&ApicidBitLayoutCtx);
	ManyDomainFinalize(&ApicidBitLayoutCtx);
}

void cpu_info::ParseCpu_CreateDomainMaskMatrix(APICID_BIT_LAYOUT_CTX *pApicidBitLayoutCtx)
{
	unsigned int DomainIndex{0};
	unsigned int PreviousBit{0};
	unsigned int NextDomainIndex;

	// Create globally identifiable masks for each domain.
	for (; DomainIndex <= pApicidBitLayoutCtx->PackageDomainIndex; DomainIndex++)
	{
		pApicidBitLayoutCtx->DomainRelativeMasks[DomainIndex][DomainIndex] = ~((1 << PreviousBit) - 1);
		PreviousBit = pApicidBitLayoutCtx->ShiftValues[DomainIndex];
	}
	// Create a relative identifier for each Domain to another higher level Domain
	for (DomainIndex = 0; DomainIndex < pApicidBitLayoutCtx->PackageDomainIndex; DomainIndex++)
	{
		/* Start to create relative IDs to the next level above the current.
		 *
		 *     A relative ID is taking the global ID mask and removing the previous mask (which is already done) and then removing the mask of
		 *     the higher level domain, so for example:
		 *
		 *     A global Logical processor mask would be 0xFFFFFFFF since all logical processors are the lowest identifier so the entire APIC ID is needed.
		 *
		 *     A global Core mask could be:  0xFFFFFFFE  meaning the Core ID doesn't include the lower Logical Processor IDs.  This will identify the 2 Logical processors as
		 *     a core globally.
		 *
		 *     A global Package mask could be:  0xFFFFFFF8  Meaning we can identify this package among other packages and this package has 8 logical processors.
		 *
		 *
		 *     To then create a Mask to create an ID relative to Package, we would do   ~(0xFFFFFFF8) & 0xFFFFFFFE  = 0x00000006  Essentially, you remove the ID mask for the upper domain
		 *     from the global mask ID for the core.  To create the full ID though you also need to use the low bit's shift value.
		 *
		 *     (APIC ID & 0x6)>>1 = CORE_ID for the Package.
		 */
		for (NextDomainIndex = DomainIndex + 1; NextDomainIndex <= pApicidBitLayoutCtx->PackageDomainIndex; NextDomainIndex++)
		{
			pApicidBitLayoutCtx->DomainRelativeMasks[DomainIndex][NextDomainIndex] = (~pApicidBitLayoutCtx->DomainRelativeMasks[NextDomainIndex][NextDomainIndex]) & (pApicidBitLayoutCtx->DomainRelativeMasks[DomainIndex][DomainIndex]);
		}
	}
}

unsigned int cpu_info::Tools_CreateTopologyShift(unsigned int count)
{
	unsigned int Shift{31u};
	unsigned int Index{(1u << Shift)};

	count = (count * 2) - 1;
	for (; Index; Index >>= 1, Shift--)
		if (count & Index)
			break;

	return Shift;
}

std::vector<unsigned int> cpu_info::Tools_GatherPlatformApicIds()
{
	// Determine X2APIC ID or fall back to APIC ID.
	auto CpuidRegisters{ReadCpuid(0, 0)};
	auto NumberOfProcessors{Tools_GetNumberOfProcessors()};
	unsigned int ApicId;
	vector<unsigned int> ApicIds;

	if (NumberOfProcessors > MAX_PROCESSORS)
		NumberOfProcessors = MAX_PROCESSORS;

	for (auto Index{0u}; Index < NumberOfProcessors; Index++)
	{
		Tools_SetAffinity(Index);
		auto id{std::min(CpuidRegisters.x.Register.Eax, 0x1Fu)};
		if (id < 0x1fu && (id = std::min(id, 0xbu)) < 0x0b)
			id = 1;
		CPUID_REGISTERS CpuidRegistersApicid{ReadCpuid(id, 0)};
		if (id == 0x1F)
		{
			if (CpuidRegistersApicid.x.Register.Ebx != 0)
				ApicId = CpuidRegistersApicid.x.Register.Edx;
			else
				CpuidRegistersApicid = ReadCpuid((id = 0x0bu), 0);
		}
		if (id == 0x0B)
		{
			if (CpuidRegistersApicid.x.Register.Ebx != 0)
				ApicId = CpuidRegistersApicid.x.Register.Edx;
			else
				CpuidRegistersApicid = ReadCpuid((id = 1), 0);
		}
		if (id == 1) // Fall back to Legacy 8 bit APIC ID.
			ApicId = (CpuidRegistersApicid.x.Register.Ebx >> 24);
		ApicIds.push_back(ApicId);
	}
	return ApicIds;
}

void cpu_info::Tools_SetAffinity(unsigned int ProcessorNumber)
{
#ifdef _WIN32
	GROUP_AFFINITY GroupAffinity{};
	unsigned short GroupIndex{0u};
	unsigned short MaxGroups{GetActiveProcessorGroupCount()};
	unsigned int NumberOfGroupProcessors;
	// Assume the active groups are going to be contiguous.
	for (; GroupIndex < MaxGroups; GroupIndex++)
		if (auto NumberOfGroupProcessors = GetActiveProcessorCount(GroupIndex);
			ProcessorNumber < NumberOfGroupProcessors)
		{
			GroupAffinity.Group = GroupIndex;
			GroupAffinity.Mask = (KAFFINITY)((ULONG64)1 << (ULONG64)ProcessorNumber);
			SetThreadGroupAffinity(GetCurrentThread(), &GroupAffinity, NULL);
			break;
		}
		else
			ProcessorNumber = ProcessorNumber - NumberOfGroupProcessors;
#elif defined linux
	// Get the size of the maximum number of configured processors.
	auto NumberOfProcessors{get_nprocs_conf()};
	// Something larger comes in we will just go with it.
	if (ProcessorNumber > NumberOfProcessors)
		NumberOfProcessors = ProcessorNumber;

	if (auto *cpu_set = CPU_ALLOC(NumberOfProcessors))
	{
		const auto SetSize = CPU_ALLOC_SIZE(NumberOfProcessors);
		CPU_ZERO_S(SetSize, cpu_set);
		CPU_SET_S(ProcessorNumber, SetSize, cpu_set);
		sched_setaffinity(getpid(), SetSize, cpu_set);
		CPU_FREE(cpu_set);
	}
#else
#endif
}

unsigned int cpu_info::Tools_GetNumberOfProcessors(void) const noexcept
{
	unsigned int NumberOfProcessors{1u};
#ifdef _WIN32
	NumberOfProcessors = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
#elif defined linux
	NumberOfProcessors = (unsigned int)get_nprocs();
#endif
	return NumberOfProcessors;
}

void cpu_info::ThreeDomainFinalize(unsigned int PackageShift, unsigned int LogicalProcessorShift)
{
	lvl_ids.resize(3);
	auto ApicIdArray{Tools_GatherPlatformApicIds()};
	unsigned int NumberOfLogicalProcessors{(unsigned)ApicIdArray.size()};
	unsigned int PackageMask{~((1u << PackageShift) - 1)};
	unsigned int LogicalProcessorPackageMask{(1u << LogicalProcessorShift) - 1};
	unsigned int CorePackageMask{((1u << PackageShift) - 1) ^ ((1u << LogicalProcessorShift) - 1)};
	unsigned int LogicalProcessorMask{(1u << LogicalProcessorShift) - 1};
	const auto fmts{getFmt()};
	string fmt{
		format("{:s}=({:s}:{:s}:{:s})", fmts.first, fmts.second, fmts.second, fmts.second)};
	/*
	 * Optionally could dynamically allocate this based on the number of logical processors.
	 */
	for (unsigned int ProcessorIndex = 0; ProcessorIndex < NumberOfLogicalProcessors; ProcessorIndex++)
	{
		const auto lv0 = ApicIdArray[ProcessorIndex] & LogicalProcessorMask;
		const auto lv1 = (ApicIdArray[ProcessorIndex] & CorePackageMask) >> LogicalProcessorShift;
		const auto lv2 = (ApicIdArray[ProcessorIndex] & PackageMask) >> PackageShift;
		cpu_ids.push_back(vformat(string_view{fmt}, make_format_args(ProcessorIndex, lv2, lv1, lv0)));
		lvl_ids[0][lv0]++;
		lvl_ids[1][lv1]++;
		lvl_ids[2][lv2]++;
	}
}

void cpu_info::ManyDomainFinalize(APICID_BIT_LAYOUT_CTX *pApicidBitLayoutCtx)
{
	unsigned int DomainShift;
	auto ApicIdArray{Tools_GatherPlatformApicIds()};
	unsigned int NumberOfLogicalProcessors{(unsigned)ApicIdArray.size()};
	unsigned int TopDomainIndex{pApicidBitLayoutCtx->PackageDomainIndex};
	unsigned int ProcessorIndex{0};
	unsigned int DomainIndex{0};
	// produce topology masks depending on what we got
	masks_names.resize(TopDomainIndex + 1);
	for (; DomainIndex <= TopDomainIndex; DomainIndex++)
		if (pApicidBitLayoutCtx->ShiftValues[DomainIndex] != 0)
		{
			auto &mn = masks_names[pApicidBitLayoutCtx->ShiftValueDomain[DomainIndex]];
			mn.first = pApicidBitLayoutCtx->DomainRelativeMasks[DomainIndex][DomainIndex];
			mn.second = DomainIndex == TopDomainIndex ? "_pkg_" : lvl_base_names[pApicidBitLayoutCtx->ShiftValueDomain[DomainIndex]];
		}
	lvl_ids.resize(masks_names.size());
	const auto fmts{getFmt()};
	for (; ProcessorIndex < ApicIdArray.size(); ProcessorIndex++)
	{
		string id{};
		for (DomainIndex = 0, DomainShift = 0; DomainIndex < TopDomainIndex; DomainIndex++)
		{
			if (pApicidBitLayoutCtx->ShiftValues[DomainIndex] != 0)
			{
				const auto index = (pApicidBitLayoutCtx->DomainRelativeMasks[DomainIndex][TopDomainIndex] & ApicIdArray[ProcessorIndex]) >> DomainShift;
				lvl_ids[DomainIndex][index]++;
				if (!id.empty())
					id.insert(0, ":");
				id.insert(0, vformat(fmts.second, make_format_args(index)));
			}
			DomainShift = pApicidBitLayoutCtx->ShiftValues[DomainIndex];
		}
		lvl_ids[DomainIndex][(pApicidBitLayoutCtx->DomainRelativeMasks[TopDomainIndex][TopDomainIndex] & ApicIdArray[ProcessorIndex]) >> pApicidBitLayoutCtx->ShiftValues[TopDomainIndex - 1]]++;
		cpu_ids.push_back(vformat(fmts.first, make_format_args(ProcessorIndex)) + format("=({:s})", id));
	}
}

pair<string, string> cpu_info::getFmt() const noexcept
{
	const auto sz{1 + (unsigned int)std::log10(Tools_GetNumberOfProcessors())};
	return {
		'{' + std::format(":0{:d}d", sz) + '}',
		'{' + std::format(":#0{:d}X", sz + 2) + '}'};
}
