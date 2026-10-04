/**
 * 	cpu_id:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class … does not work.
 *
 *  This is the header of the cpu_id subclass, also implementing the cpuid
 *  instruction operations.
 */
#pragma once

#include <cpu_info_types.hpp>

#include <string>
#include <cstdint>

namespace cpu_info
{
	/**
	 * 	Result of a cpuid - instruction (simply 4 32bit registers)
	 */
	union cpuid_result
	{
		unsigned int r[ 4 ];
		struct
		{
			unsigned int ax, bx, cx, dx;
		} e;
	};

	/**
	 * 	This class is made to describe one logical CPU in your system
	 * 	→ its APIC_ID
	 * 	→ a list of this id masked and shifted as different domain IDs
	 *
	 *	The CPUID leafs are all queried upon creation!  Thus, the CURRENT LOGICAL CPU
	 *	is queried.  Please make sure, thread affinity is bound to this logical cpu.
	 *
	 *	Why derive from map?
	 *	→	Maps create a new entry upon using the []-operator - BUT WE MUST NOT do that.
	 *		So the operator needs an overload to neither throw, nor create an invalid entry.
	 *		Our overload simply returns an invalid entry.
	 */
	class cpu_id : public map< unsigned, vector< cpuid_result > >
	{
	  public:
		// actually execute cpuid( leaf, subleaf )
		static cpuid_result cpuid( unsigned Leaf, unsigned Subleaf ) noexcept;

		using M = map< unsigned, vector< cpuid_result > >;
		using L = vector< cpuid_result >;

		cpu_id();

		L &operator[]( unsigned leaf ) noexcept
		{
			if ( contains( leaf ) ) return M::operator[]( leaf );
			else return _invalid;
		}
		const L &operator[]( unsigned leaf ) const noexcept
		{
			if ( contains( leaf ) ) return at( leaf );
			else return _invalid;
		}
		unsigned		   max_leaf() const noexcept { return _maxLeaf; }

						   operator bool() const noexcept { return !empty(); }
						   operator apic_id() const noexcept;
		apic_id			   id( cpu_domain domain = LogicalDomain ) const noexcept;
		bool			   operator()( cpu_feature feature ) const noexcept;
		uint8_t			   stepping() const noexcept;
		uint8_t			   family() const noexcept;
		uint8_t			   model() const noexcept;
		cpu_processor_type type() const noexcept;
		cpu_core_type	   core_type() const noexcept;
		unsigned		   core_model() const noexcept;
		string			   brand_string() const noexcept;
		cpu_efficiency	   efficiency() const noexcept;

	  protected:
		L			  &retrieve( unsigned leaf ) noexcept;
		static L	   _invalid;
		const unsigned _maxLeaf{ 0 }; // initialized upon construction!
	};
} // namespace cpu_info
