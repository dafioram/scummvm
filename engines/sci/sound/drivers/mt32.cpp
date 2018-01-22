/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/system.h"
#include "sci/sound/drivers/genmidi.h"
#include "sci/sound/drivers/mt32.h"

namespace Sci {

static bool isGeneralMidiPatch(Resource &patchData) {
	uint size = patchData.size();

	if (size < 1155) {
		return false;
	}

	// The maximum possible size of an MT-32 patch is 16889, plus some Mac
	// games' patches (e.g. LSL5) have an extra byte at the end
	if (size > 16890) {
		return true;
	}

	if (patchData.getUint16LEAt(1153) + 1155 == size) {
		return true;
	}

	enum { kNumTimbresOffset = 491 };
	if (patchData[kNumTimbresOffset] > 64) {
		return true;
	}

	return false;
}

Mt32Driver::Mt32Driver(ResourceManager &resMan, const SciVersion version) :
	SoundDriver(resMan, version),
	_deviceId(12),
	_reverbMode(0xff),
	_masterVolume(15) {

	MidiDriver::DeviceHandle dev = MidiDriver::detectDevice(MDT_MIDI | MDT_PREFER_MT32);
	_device.reset(MidiDriver::createMidi(dev));
	assert(_device);
	const int errNo = _device->open();
	if (errNo != 0) {
		error("Failure opening MT-32 device: %s", _device->getErrorName(errNo));
	}

	for (int i = 0; i < kNumChannels; ++i) {
		if (i == kPercussionChannel) {
			_channels[i].hw = static_cast<MidiChannel_MPU401 *>(_device->getPercussionChannel());
		} else {
			_channels[i].hw = static_cast<MidiChannel_MPU401 *>(_device->allocateChannel());
		}
	}

	_isEmulated = (MidiDriver::getDeviceString(dev, MidiDriver::kDriverId) == "mt32");

	Resource *patchData = resMan.findResource(ResourceId(kResourceTypePatch, 1), false);
	if (!patchData) {
		// TODO: Read from MT32.DRV for the very early games
		error("Could not find MT-32 patch data");
	}

	// MT-32 patch contents:
	// - 0-19        after-SysEx message
	// - 20-39       before-SysEx message
	// - 40-59       goodbye SysEx message
	// - 60-61       volume
	// - 62          reverb
	// - 63-73       reverb Sysex message
	// - 74-106      [3 * 11] reverb data
	// - 107-490     [256 + 128] patches 1-48
	// --> total: 491 bytes
	// - 491         number of timbres (64 max)
	// - 492..n      [246 * number of timbres] timbre data
	// - n-n+1       flag (0xabcd)
	// - n+2-n+385   [256 + 128] patches 49-96
	// - n+386-n+387 flag (0xdcba)
	// - n+388-n+643 rhythm key map
	// - n+644-n+652 partial reserve

	if (_version <= SCI_VERSION_01) {
		_defaultReverbMode = patchData->getUint8At(62);
	}

	patchData->subspan(40, _goodbyeSysEx.size()).unsafeCopyDataTo(_goodbyeSysEx.data());

	for (uint j = 0; j < _reverbModes[0].size(); ++j) {
		for (uint i = 0; i < _reverbModes.size(); ++i) {
			_reverbModes[i][j] = patchData->getUint8At(74 + j * _reverbModes.size() + i);
		}
	}

	// Message displayed at start of SysEx transfer
	sendSysEx(kDisplayAddress, patchData->subspan(20, 20), _isEmulated);

	// Patches 1-48
	uint32 patchAddress = kPatchAddress;
	sendPatches(patchAddress, patchData->subspan(107, kPatchDataSize));

	const uint8 numTimbres = patchData->getUint8At(491);
	assert(numTimbres <= 64);
	sendTimbres(numTimbres, patchData->subspan(492, numTimbres * kTimbreDataSize));

	SysEx extraData = patchData->subspan(492 + numTimbres * kTimbreDataSize);
	uint16 flag = 0;
	if (extraData.size() > 2) {
		extraData.getUint16BEAt(0);
		extraData += 2;
	}

	if (flag == 0xabcd) {
		// Patches 49-96
		sendPatches(patchAddress, extraData.subspan(0, kPatchDataSize));
		extraData += kPatchDataSize;
		if (extraData.size() > 2) {
			flag = extraData.getUint16BEAt(0);
			extraData += 2;
		}
	}

	if (flag == 0xdcba) {
		uint32 address = kRhythmKeyMapAddress;
		SysEx rhythmData = extraData.subspan(0, kRhythmDataSize + kPartialReserveSize);
		for (int i = 0; i < kNumRhythmPatches; ++i) {
			sendCountingSysEx(address, rhythmData, kRhythmPatchSize);
		}
		sendSysEx(kPartialReserveAddress, rhythmData.subspan(0, kPartialReserveSize), _isEmulated);
	}

	// Message displayed at game startup
	sendSysEx(kDisplayAddress, patchData->subspan(0, 20), _isEmulated);

	const byte disableCm32P[] = { 0x16, 0x16, 0x16, 0x16, 0x16, 0x16 };
	sendSysEx(kDisableCm32PAddress, SysEx(disableCm32P, sizeof(disableCm32P)), _isEmulated);

	setMasterVolume(12);
}

Mt32Driver::~Mt32Driver() {
	sendSysEx(kDisplayAddress, SysEx(_goodbyeSysEx.data(), _goodbyeSysEx.size()), true);
	_device->close();
}

void Mt32Driver::noteOff(const uint8 channelNo, const uint8 note, const uint8 velocity) {
	_channels[channelNo].hw->noteOff(note, velocity);
	debugC(kDebugLevelSound, "Off %2d n %3d v %3d", channelNo, note, velocity);
}

void Mt32Driver::noteOn(const uint8 channelNo, const uint8 note, const uint8 velocity) {
	Channel &channel = _channels[channelNo];
	channel.enabled = true;
	debugC(kDebugLevelSound, "On  %2d n %3d v %3d", channelNo, note, velocity);
	channel.hw->noteOn(note, velocity);
}

void Mt32Driver::controllerChange(const uint8 channelNo, const uint8 controllerNo, const uint8 value) {
	Channel &channel = _channels[channelNo];
	switch (controllerNo) {
	case kModulationController:
		if (value == channel.modulation) {
			return;
		}
		channel.modulation = value;
		break;
	case kVolumeController:
		if (value == channel.volume) {
			return;
		}
		channel.volume = value;
		break;
	case kPanController:
		if (value == channel.pan) {
			return;
		}
		channel.pan = value;
		break;
	case kDamperPedalController:
		if (value == channel.damperPedalOn) {
			return;
		}
		channel.damperPedalOn = (value != 0);
		break;
	case kAllNotesOffController:
		if (!channel.enabled) {
			return;
		}
		channel.enabled = false;
		break;
	case kReverbModeController:
		if (_version <= SCI_VERSION_01) {
			setReverbMode(value);
		}
		return;
	default:
		return;
	}

	debugC(kDebugLevelSound, "CC  %2d n %3d v %3d", channelNo, controllerNo, value);
	channel.hw->controlChange(controllerNo, value);
}

void Mt32Driver::programChange(const uint8 channelNo, const uint8 programNo) {
	Channel &channel = _channels[channelNo];
	if (channel.program == programNo) {
		return;
	}
	channel.program = programNo;
	debugC(kDebugLevelSound, "PC  %2d p %3d", channelNo, programNo);
	channel.hw->programChange(programNo);
}

void Mt32Driver::pitchBend(const uint8 channelNo, const uint16 bend) {
	Channel &channel = _channels[channelNo];
	if (channel.pitchBend == bend) {
		return;
	}
	channel.pitchBend = bend;
	debugC(kDebugLevelSound, "PB  %2d p %04x", channelNo, bend);
	channel.hw->pitchBend(bend - 0x2000);
}

void Mt32Driver::setReverbMode(uint8 modeNo) {
	if (_version <= SCI_VERSION_01 && modeNo == kUseDefaultReverb) {
		modeNo = _defaultReverbMode;
	}

	if (modeNo == _reverbMode) {
		return;
	}

	_reverbMode = modeNo;

	const byte *data = _reverbModes[modeNo].data();
	sendSysEx(kReverbModeAddress, SysEx(data, _reverbModes[modeNo].size()), true);
}

void Mt32Driver::setMasterVolume(const uint8 volume) {
	_masterVolume = volume;
	sendMasterVolume(volume);
}

void Mt32Driver::enable(const bool enabled) {
	SoundDriver::enable(enabled);
	if (enabled) {
		sendMasterVolume(_masterVolume);
	} else {
		sendMasterVolume(0);
	}
}

void Mt32Driver::debugPrintState(Console &con) const {
	con.debugPrintf("Channels:\n\n");
	for (int i = 0; i < kNumChannels; ++i) {
		const Channel &channel = _channels[i];
		if (channel.program != kUnmapped) {
			// modulation is excluded because it is never sent to the device
			con.debugPrintf("%2d: prog %u bend %04x pan %u vol %u dp %d%s\n",
							i,
							channel.program,
							channel.pitchBend,
							channel.pan,
							channel.volume,
							channel.damperPedalOn,
							channel.enabled ? "" : ", disabled");
		} else {
			con.debugPrintf("%2d: unmapped\n", i);
		}
	}
}

void Mt32Driver::sendSysEx(uint32 address, const SysEx &data, const bool skipDelays) {
	enum {
		kMaxPacketSize = 0x100,
		kHeaderSize = 7,
		kNonDataSize = kHeaderSize + /* sysex command + checksum + end of sysex */ 3,
		kMaxDataSize = kMaxPacketSize - kNonDataSize,
		kManufacturerId = 0x41,
		kDeviceId = 0x10,
		kModelId = 0x16,
		kCommandId = 0x12
	};

	byte buffer[kMaxDataSize + kHeaderSize + /* checksum */ 1];
	buffer[0] = kManufacturerId;
	buffer[1] = kDeviceId;
	buffer[2] = kModelId;
	buffer[3] = kCommandId;

	const byte *source = data.data();
	const uint size = data.size();
	assert(size < kMaxDataSize);

	buffer[4] = (address >> 16) & 0xff;
	buffer[5] = (address >> 8) & 0xff;
	buffer[6] = address & 0xff;

	uint16 checksum = 0;
	for (uint i = 4; i < kHeaderSize; ++i) {
		checksum -= buffer[i];
	}
	for (uint i = kHeaderSize; i < kHeaderSize + size; ++i) {
		const byte b = *source++;
		buffer[i] = b;
		checksum -= b;
	}
	buffer[kHeaderSize + size] = checksum & 0x7f;

	_device->sysEx(buffer, kHeaderSize + size + 1);

	if (!skipDelays) {
		enum { kMt32Rev00BufferOverflowAvoidance = 40 };
		const uint32 delay = (size + kNonDataSize) * 1000 / 3125 +
			kMt32Rev00BufferOverflowAvoidance;
		g_system->updateScreen();
		g_sci->sleep(delay);
	}
}

void Mt32Driver::sendCountingSysEx(uint32 &address, SysEx &data, const uint8 size) {
	sendSysEx(address, data.subspan(0, size), _isEmulated);
	address += size;
	data += size;
	if (address & 0x80) {
		address += 0x100 - 0x80;
	}
}

void Mt32Driver::sendPatches(uint32 &address, SysEx data) {
	for (int i = 0; i < kNumPatchesPerBank; ++i) {
		sendCountingSysEx(address, data, kPatchSize);
	}
}

void Mt32Driver::sendTimbres(const uint8 numTimbres, SysEx data) {
	for (int i = 0; i < numTimbres; ++i) {
		uint32 address = kTimbreAddress + i * 0x200;
		sendCountingSysEx(address, data, kShortTimbreSize);
		for (int j = 0; j < kNumLongTimbres; ++j) {
			sendCountingSysEx(address, data, kLongTimbreSize);
		}
	}
}

void Mt32Driver::sendMasterVolume(uint8 volume) {
	// SSCI used a LUT, we don't need that extra bit of performance so we just
	// calculate the value
	volume = (volume * 100 + (kMaxMasterVolume / 2)) / kMaxMasterVolume;
	const byte data[] = { volume };
	sendSysEx(kMasterVolumeAddress, SysEx(data, sizeof(data)), true);
}

SoundDriver *makeMt32Driver(ResourceManager &resMan, const SciVersion version) {
	Resource *patchData = resMan.findResource(ResourceId(kResourceTypePatch, 1), false);
	if (version >= SCI_VERSION_2 ||
		(patchData && isGeneralMidiPatch(*patchData))) {
		return makeGeneralMidiDriver(resMan, version, true);
	}

	return new Mt32Driver(resMan, version);
}

} // End of namespace Sci
