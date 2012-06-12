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

// vim: set noexpandtab ts=4 sw=4:

#ifndef PC_OS_MODEL_H
#define PC_OS_MODEL_H

#include <boost/detail/endian.hpp>
#include <stdint.h>

#include "external_interface/default_return_values.h"
#include "com_isense_radio.h"
#include "pc_clock.h"
#include "pc_debug.h"
#include "pc_rand.h"
#include "pc_timer.h"
#include "pc_com_uart.h"
#include "util/serialization/endian.h"

namespace wiselib {
	class PCOsModel
		: public DefaultReturnValues<PCOsModel>
	{
		public:
	      int argc;
			const char** argv;

			typedef PCOsModel AppMainParameter;
			typedef PCOsModel Os;
			
			typedef uint32_t size_t;
			typedef uint8_t block_data_t;
			
			typedef PCClockModel<PCOsModel> Clock;
			typedef PCDebug<PCOsModel> Debug;
			
			// Radio model can only exist when port for communication with the
			// isense node is known so it has to be instantiated by the user
			
			typedef PCRandModel<PCOsModel> Rand;
			typedef PCTimerModel<PCOsModel, 100> Timer;
			
			typedef PCComUartModel<PCOsModel, true> ISenseUart;
			typedef PCComUartModel<PCOsModel, false> Uart;
			typedef ComISenseRadioModel<PCOsModel, ISenseUart> Radio;
			
		//	typedef MallocFreeAllocator<PCOsModel> Allocator;
			
			static const Endianness endianness = WISELIB_ENDIANNESS;
		//	static typename Allocator::self_pointer_t allocator;
	};
} // ns wiselib

#endif // PC_OS_MODEL_H

