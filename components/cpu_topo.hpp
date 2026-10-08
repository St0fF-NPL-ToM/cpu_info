#pragma once
/**
 *
 */
#include "cpu_id.hpp"

namespace cpu_info
{

	class cpu_topo
	{
	  public:
		// system-assigned process affinity mask
		const affinity				  process_affinity;
		// list of actual cpu_id mappings, including pre-masked IDs
		std::vector< cpu_id >			  cpu_ids;
		// a vector of maps to count masked apic_ids - describes how many
		// logical cores share the respective masked apic id
		std::vector< std::map< APIC_id, int > > lvl_ids;

		cpu_topo() // throws in case affinity switching fails
			: process_affinity( affinity().read_current() )
		{
			if ( !process_affinity.inherit( 0 ).set_to_current() ) // check if switching works
				throw "cannot switch cpu affinity, no fallback available.";
			process_affinity.set_to_current(), refresh();
		}
		// or thread affinity cannot be set.
		void refresh() noexcept
		{
			cpu_ids.clear();
			{
				affinity_iterator affinity;
				do cpu_ids.emplace_back();
				while ( ++affinity );
			} // end of block resets affinity back to what it was before
			parse_topology();
		}
		// returns the number of logical cpus accessible to the callee (without further OS calls)
		size_t	   count() const noexcept { return cpu_ids.size(); }
		cpu_domain max_domain() const noexcept { return cpu_domain( lvl_ids.size() ); }
		// count items of a specific domain (like logical cpu count, core count, tile, package)
		size_t	   count_domain( cpu_domain lvl ) const noexcept
		{
			if ( lvl == cpu_domain::InvalidDomain || lvl_ids.size() < ( size_t ) lvl ) return 1;
			return lvl_ids[ lvl - 1 ].size();
		}
		// throwing accessor, throws in case of invalid index
		const cpu_id &operator[]( size_t index ) const noexcept { return cpu_ids[ index ]; }

		bool		  knows_efficiency() const noexcept
		{ return cpu_ids[ 0 ].efficiency() != cpu_efficiency::unknownEff; }

	  private: // internal operations
		class mask_map : public std::map< unsigned, id_mask >
		{
		  public:
			using BASE = std::map< unsigned, id_mask >;
			using BASE::map;
			id_mask operator[]( unsigned key ) const
			{
				if ( contains( key ) ) return at( key );
				else return 0u;
			}
		};
		struct apicid_bit_layout
		{
			cpu_domain domain{ InvalidDomain };
			unsigned   shift{};
			mask_map   relative_masks;
		};
		class apicid_bit_layouts : public std::vector< apicid_bit_layout >
		{
			static constexpr unsigned number_of_apic_bits = 32;

		  public:
			cpu_domain		   top_domain{ InvalidDomain };
			unsigned		   domains() const { return size() + 1; }

			// non-const access operator shall emplace/resize on demand!
			apicid_bit_layout &operator[]( size_t index )
			{
				while ( index >= size() )
					emplace_back( ( cpu_domain ) ( back().domain + 1 ), 0u, mask_map{} );
				return vector< apicid_bit_layout >::operator[]( index );
			}
			// likewise
			const apicid_bit_layout operator[]( size_t index ) const
			{
				if ( index < size() ) return vector< apicid_bit_layout >::operator[]( index );
				else return { InvalidDomain, 0, mask_map{} };
			}
		};
		void parse_topology() noexcept
		{
			apicid_bit_layouts abl;
			auto			  &cpu0 = cpu_ids.front();
			if ( const auto il = cpu0.id_leaf(); il <= 1 )
			{
				abl.emplace_back( LogicalDomain, cpu0.domain_shift( LogicalDomain ), mask_map{} );
				abl.emplace_back( CoreDomain, cpu0.domain_shift( CoreDomain ), mask_map{} );
				abl.top_domain = ModuleDomain;
			} else
			{
				const auto &sl = cpu0[ il ];
				for ( unsigned sub{ 0 }; sl[ sub ].e.bx != 0; ++sub )
				{
					// CPUID.B or 1F.x.ECX[15:8] = Level Type / Domain Type
					const auto DomainType  = ( sl[ sub ].e.cx >> 8 ) & 0xFF;
					// CPUID.B or 1F.x.EAX[4:0] = Level Shift / Domain Shift
					const auto DomainShift = sl[ sub ].e.ax & 0x1F;
					/*
					 * Best to check for known domains explicity since
					 * the ones you use may not be in sequential ordering.
					 */
					switch ( DomainType )
					{
						case InvalidDomain:
							/*  This would be an error, could log it. */
						case LogicalDomain:
						case CoreDomain:
						case ModuleDomain:
						case TileDomain:
						case DieDomain:
						case DieGrpDomain:
							abl.emplace_back( ( cpu_domain ) DomainType, DomainShift, mask_map{} );
							break;
							// First Domain is always Logical Processor,
							// so we will always have a valid previous.
						default:
							abl.back().shift = DomainShift;
							abl.top_domain	 = ( cpu_domain ) DomainType;
					}
				}
			}
			// second step: produce relative apic_id_masks from retrieved information
			unsigned	index{}, nxt_index{}, prev_bit{}, top_domain{ ( unsigned ) abl.size() };
			unsigned	domain_shift, cpu_cnt{ ( unsigned ) cpu_ids.size() };
			const auto &ca{ abl };
			for ( ; index < top_domain; ++index )
			{ // previous shift makes up current mask (see level_mask() implementation)
				abl[ index ].relative_masks.emplace( index, ~( ( 1 << prev_bit ) - 1 ) );
				prev_bit = ca[ index ].shift;
			}
			for ( index = 0u; index < top_domain; ++index )
				for ( nxt_index = index + 1; nxt_index <= top_domain; ++nxt_index )
					abl[ index ].relative_masks.emplace(
						nxt_index,
						( ~ca[ nxt_index ].relative_masks[ nxt_index ] )
							& ( ca[ index ].relative_masks[ index ] ) );

			// at last, build the counter-map
			lvl_ids.clear(), lvl_ids.resize( top_domain + 1 );
			for ( unsigned cpu{}; cpu < cpu_cnt; cpu++ )
			{
				for ( index = 0, domain_shift = 0; index < top_domain; index++ )
				{
					if ( ca[ index ].shift != 0 )
						lvl_ids[ index ][ ( ca[ index ].relative_masks[ top_domain ]
											& ( APIC_id ) cpu_ids[ cpu ] )
										  >> domain_shift ]++;
					domain_shift = abl[ index ].shift;
				}
				lvl_ids[ index ][ ( ca[ top_domain ].relative_masks[ top_domain ]
									& ( APIC_id ) cpu_ids[ cpu ] )
								  >> ca[ top_domain - 1 ].shift ]++;
			}
		}
	};

} // namespace cpu_info
