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

#include "xsvf_player.hpp"

#include <array>

#if defined(JTAG_DEBUG)
#include <cstdio>
#define JTAG_TAP_DEBUG 1
#define tap_print(...) do { printf(__VA_ARGS__); } while(0)
#else
#define JTAG_TAP_DEBUG 0
#define tap_print(...) do { } while(0)
#endif

namespace jtag {
namespace tap {

XSVFPlayer::XSVFPlayer(
	const uint8_t* const data,
	TAPMachine& tap
) : p { data },
	tap { tap }
{
	while( !done ) {
		const auto instruction = get_instruction();
		handle(instruction);
	}
}

void XSVFPlayer::x_complete() {
	done = true;
	tap_print("XCOMPLETE\n");
}

void XSVFPlayer::x_tdo_mask() {
	tdo_mask = read_bits(dr_size);
	tap_print("XTDOMASK %s\n", tdo_mask.to_str().c_str());
}

void XSVFPlayer::x_s_ir() {
	const auto bits_count = read_u8();
	const auto tdi_value = read_bits(bits_count);
	tap_print("XSIR %s\n", tdi_value.to_str().c_str());
	if( tap.shift_ir(tdi_value) ) {
		abort();
	}
}

void XSVFPlayer::x_run_test() {
	const auto value = read_u32();
	tap_print("XRUNTEST %u\n", value);
	tap.set_run_test(value);
}

void XSVFPlayer::x_repeat() {
	const auto value = read_u8();
	tap_print("XREPEAT %u\n", value);
	tap.set_repeat(value);
}

void XSVFPlayer::x_s_dr_size() {
	dr_size = read_u32();
	tdo_mask = { };
	tap_print("XSDRSIZE %u\n", dr_size);
}

void XSVFPlayer::x_s_dr_tdo() {
	const auto tdi_value = read_bits(dr_size);
	const auto tdo_expected = read_bits(dr_size);
	tap_print("XSDRTDO %s %s %s\n", tdi_value.to_str().c_str(), tdo_expected.to_str().c_str(), tdo_mask.to_str().c_str());
	if( tap.shift_dr(tdi_value, tdo_expected, tdo_mask) ) {
		abort();
	}
}

void XSVFPlayer::x_state() {
	const auto state = read_state();
	tap_print("XSTATE %s\n", c_str(state));
	tap.state(state);
}

void XSVFPlayer::x_end_ir() {
	const auto value = read_u8();
	if( value > 1 ) {
		abort();
	}
	const auto state = (value == 1) ? state_t::pause_ir : state_t::run_test_idle;
	tap_print("XENDIR %s\n", c_str(state));
	tap.set_end_ir(state);
}

void XSVFPlayer::x_end_dr() {
	const auto value = read_u8();
	if( value > 1 ) {
		abort();
	}
	const auto state = (value == 1) ? state_t::pause_dr : state_t::run_test_idle;
	tap_print("XENDDR %s\n", c_str(state));
	tap.set_end_dr(state);
}

void XSVFPlayer::x_wait() {
	const auto wait_state = read_state();
	const auto end_state = read_state();
	const auto wait_time = read_u32();
	tap_print("XWAIT %s %s %u\n", c_str(wait_state), c_str(end_state), wait_time);
	tap.wait(wait_state, end_state, wait_time);
}

void XSVFPlayer::unimplemented() {
	abort();
}

uint8_t XSVFPlayer::read_u8() {
	return *(p++);
}

state_t XSVFPlayer::read_state() {
	return static_cast<state_t>(read_u8());
}

uint32_t XSVFPlayer::read_u32() {
	uint32_t result = read_u8();
	result <<= 8;
	result |= read_u8();
	result <<= 8;
	result |= read_u8();
	result <<= 8;
	result |= read_u8();
	return result;
}

bits_t XSVFPlayer::read_bits(const size_t count) {
	const bits_t result { p, count };
	p += (count + 7) >> 3;
	return result;
}

XSVFPlayer::instruction_t XSVFPlayer::get_instruction() {
	const auto instruction = read_u8();
	if( instruction >= handler.size() ) {
		abort();
	}
	return instruction;
}

void XSVFPlayer::handle(const instruction_t instruction) {
	const auto method_pointer = handler[instruction];
	if( method_pointer == &XSVFPlayer::unimplemented ) {
		abort();
	}
	(this->*method_pointer)();
}

void XSVFPlayer::abort() {
	tap_print("ABORT\n");
	error = true;
	done = true;
}

const std::array<XSVFPlayer::handler_t, 0x18> XSVFPlayer::handler { {
	/* 0x00 */ &XSVFPlayer::x_complete,
	/* 0x01 */ &XSVFPlayer::x_tdo_mask,
	/* 0x02 */ &XSVFPlayer::x_s_ir,
	/* 0x03 */ &XSVFPlayer::unimplemented,
	/* 0x04 */ &XSVFPlayer::x_run_test,
	/* 0x05 */ &XSVFPlayer::unimplemented,
	/* 0x06 */ &XSVFPlayer::unimplemented,
	/* 0x07 */ &XSVFPlayer::x_repeat,
	/* 0x08 */ &XSVFPlayer::x_s_dr_size,
	/* 0x09 */ &XSVFPlayer::x_s_dr_tdo,
	/* 0x0a */ &XSVFPlayer::unimplemented,
	/* 0x0b */ &XSVFPlayer::unimplemented,
	/* 0x0c */ &XSVFPlayer::unimplemented,
	/* 0x0d */ &XSVFPlayer::unimplemented,
	/* 0x0e */ &XSVFPlayer::unimplemented,
	/* 0x0f */ &XSVFPlayer::unimplemented,
	/* 0x10 */ &XSVFPlayer::unimplemented,
	/* 0x11 */ &XSVFPlayer::unimplemented,
	/* 0x12 */ &XSVFPlayer::x_state,
	/* 0x13 */ &XSVFPlayer::x_end_ir,
	/* 0x14 */ &XSVFPlayer::x_end_dr,
	/* 0x15 */ &XSVFPlayer::unimplemented,
	/* 0x16 */ &XSVFPlayer::unimplemented,
	/* 0x17 */ &XSVFPlayer::x_wait,
} };

} /* namespace tap */
} /* namespace jtag */
