/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __XSVF_PLAYER_H__
#define __XSVF_PLAYER_H__

#include "jtag_tap.hpp"

#include <cstdint>
#include <cstddef>
#include <array>

namespace jtag {
namespace tap {

class XSVFPlayer {
public:
	XSVFPlayer(const uint8_t* const data, TAPMachine& tap);

	bool failed() const { return error; }

private:
	enum class instruction_t : uint8_t {
		XCOMPLETE    = 0x00,
		XTDOMASK     = 0x01,
		XSIR         = 0x02,
		XSDR         = 0x03,
		XRUNTEST     = 0x04,
		XREPEAT      = 0x07,
		XSDRSIZE     = 0x08,
		XSDRTDO      = 0x09,
		XSETSDRMASKS = 0x0a,
		XSDRINC      = 0x0b,
		XSDRB        = 0x0c,
		XSDRC        = 0x0d,
		XSDRE        = 0x0e,
		XSDRTDOB     = 0x0f,
		XSDRTDOC     = 0x10,
		XSDRTDOE     = 0x11,
		XSTATE       = 0x12,
		XENDIR       = 0x13,
		XENDDR       = 0x14,
		XSIR2        = 0x15,
		XCOMMENT     = 0x16,
		XWAIT        = 0x17,
	};

	using handler_t = void (XSVFPlayer::*)();
	static const std::array<handler_t, 0x18> handler;

	const uint8_t* p;
	TAPMachine& tap;
	uint32_t dr_size { 0 };
	bits_t tdo_mask { };
	bool error { false };
	bool done { false };

	void x_complete();
	void x_tdo_mask();
	void x_s_ir();
	void x_run_test();
	void x_repeat();
	void x_s_dr_size();
	void x_s_dr_tdo();
	void x_state();
	void x_end_ir();
	void x_end_dr();
	void x_wait();

	void unimplemented();

	uint8_t read_u8();
	state_t read_state();
	uint32_t read_u32();
	bits_t read_bits(const size_t count);

	instruction_t get_instruction();
	void handle(const instruction_t instruction);

	void abort();
};

} /* namespace tap */
} /* namespace jtag */

#endif/*__XSVF_PLAYER_H__*/
