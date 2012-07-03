/***************************************************************************
** This file is part of the generic algorithm library Wiselib.           **
** Copyright (C) 2008,2009 by the Wisebed (www.wisebed.eu) project.      **
**                                                                       **
** The Wiselib is free software: you can redistribute it and/or modify   **
** it under the terms of the GNU Lesser General Public License as        **
** published by the Free Software Foundation, either version 3 of the    **
** License, or (at your option) any later version.                       **
**                                                                       **
** The Wiselib is distributed in the hope that it will be useful,        **
** but WITHOUT ANY WARRANTY; without even the implied warranty of        **
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
** GNU Lesser General Public License for more details.                   **
**                                                                       **
** You should have received a copy of the GNU Lesser General Public      **
** License along with the Wiselib.                                       **
** If not, see <http://www.gnu.org/licenses/>.                           **
***************************************************************************/
#ifndef __ALGORITHMS_6LOWPAN_IPV6_STACK_H__
#define __ALGORITHMS_6LOWPAN_IPV6_STACK_H__

#include "algorithms/6lowpan/icmpv6.h"
#include "algorithms/6lowpan/udp.h"
#include "algorithms/6lowpan/ipv6.h"
#include "algorithms/6lowpan/lowpan.h"
#include "algorithms/6lowpan/ipv6_packet_pool_manager.h"

namespace wiselib
{

	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P>
	class IPv6Stack
	{
	public:
	
	typedef OsModel_P OsModel;
	typedef Radio_P Radio;
	typedef Debug_P Debug;
	typedef Timer_P Timer;
	
	typedef wiselib::LoWPAN<OsModel, Radio, Debug, Timer> LoWPAN_t;
	typedef wiselib::IPv6<OsModel, LoWPAN_t, Debug, Timer> IPv6_t;
	typedef wiselib::UDP<OsModel, IPv6_t, LoWPAN_t, Debug> UDP_t;
	typedef wiselib::ICMPv6<OsModel, IPv6_t, LoWPAN_t, Debug> ICMPv6_t;
	typedef wiselib::IPv6PacketPoolManager<OsModel, IPv6_t, LoWPAN_t, Debug> Packet_Pool_Mgr_t;
	
	void init( Radio& radio, Debug& debug, Timer& timer)
	{
		radio_ = &radio;
		debug_ = &debug;
		timer_ = &timer;
		
		debug_->debug( "IPv6 stack init: %d\n", radio_->id());
		
		packet_pool_mgr.init( *debug_ );
	
		//Init LoWPAN
		lowpan.init(*radio_, *debug_, &packet_pool_mgr, *timer_, ipv6);
	 
		//Init IPv6
		ipv6.init( lowpan, *debug_, &packet_pool_mgr, *timer_);
		//IPv6 will enable lower level radios
		ipv6.enable_radio();
		
		//Init UDP
		udp.init( ipv6, *debug_, &packet_pool_mgr);
		//Just register callback, not enable IP radio
		udp.enable_radio();
		
		//Init ICMPv6
		icmpv6.init( ipv6, *debug_, &packet_pool_mgr);
		//Just register callback, not enable IP radio
		icmpv6.enable_radio();
	}
	
	ICMPv6_t icmpv6; 
	UDP_t udp;
	IPv6_t ipv6;
	LoWPAN_t lowpan;
	Packet_Pool_Mgr_t packet_pool_mgr;
	
	
	private:
	typename Radio::self_pointer_t radio_;
	typename Debug::self_pointer_t debug_;
	typename Timer::self_pointer_t timer_;
	};
}
#endif
