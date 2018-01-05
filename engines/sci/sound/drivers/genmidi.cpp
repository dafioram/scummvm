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

#include "audio/mididrv.h"
#include "sci/sound/drivers/genmidi.h"

namespace Sci {

Sci1GeneralMidiDriver::Sci1GeneralMidiDriver(ResourceManager &resMan, const SciVersion version, const bool isMt32) :
	Sci1SoundDriver(resMan, version),
	_reverbMode(0),
	_masterVolume(15) {

	MidiDriver::DeviceHandle dev = MidiDriver::detectDevice(MDT_MIDI | (isMt32 ? MDT_PREFER_MT32 : MDT_PREFER_GM));
	_device.reset(MidiDriver::createMidi(dev));
	assert(_device);
	const int errNo = _device->open();
	if (errNo != 0) {
		error("Failure opening General MIDI device: %s", _device->getErrorName(errNo));
	}

	for (int i = 0; i < kNumChannels; ++i) {
		if (i == kPercussionChannel) {
			_channels[i].hw = static_cast<MidiChannel_MPU401 *>(_device->getPercussionChannel());
		} else {
			_channels[i].hw = static_cast<MidiChannel_MPU401 *>(_device->allocateChannel());
		}
	}

	if (version >= SCI_VERSION_2) {
		_deviceId = isMt32 ? 12 : 7;
	} else {
		if (isMt32) {
			error("General MIDI driver can only be used for MT-32 in SCI32");
		}

		if (version >= SCI_VERSION_1_LATE) {
			_deviceId = 12;
		} else {
			error("Unimplemented SCI sound version %d", version);
		}
	}

	Resource *patchData = resMan.findResource(ResourceId(kResourceTypePatch, isMt32 ? 1 : 4), false);
	SciSpan<const byte> midiData;
	if (patchData) {
		patchData->subspan(0, _programMap.size()).unsafeCopyDataTo(_programMap.data());
		patchData->subspan(128, _noteShift.size()).unsafeCopyDataTo(_noteShift.data());
		patchData->subspan(256, _volumeShift.size()).unsafeCopyDataTo(_volumeShift.data());
		patchData->subspan(384, _percussionMap.size()).unsafeCopyDataTo(_percussionMap.data());
		_channels[kPercussionChannel].volumeShift = patchData->getInt8At(512);
		patchData->subspan(513, _programVelocityMap.size()).unsafeCopyDataTo(_programVelocityMap.data());
		for (int i = 0; i < kNumVelocityMaps; ++i) {
			patchData->subspan(641 + (i * _velocityMaps[0].size()), _velocityMaps[0].size()).unsafeCopyDataTo(_velocityMaps[i].data());
		}
		midiData = patchData->subspan(1153 + sizeof(uint16), patchData->getUint16LEAt(1153));
	} else if (!isMt32) {
		warning("No GM patch data found, using defaults");
		for (int i = 0; i < kNumPrograms; ++i) {
			_programMap[i] = i;
		}
		Common::fill(_noteShift.begin(), _noteShift.end(), 0);
		Common::fill(_volumeShift.begin(), _volumeShift.end(), 0);
		for (int i = 0; i < kNumNotes; ++i) {
			_percussionMap[i] = i;
		}
		_percussionVolumeShift = 0;
		Common::fill(_programVelocityMap.begin(), _programVelocityMap.end(), 0);
		for (int i = 0; i < kNumVelocities; ++i) {
			_velocityMaps[0][i] = i;
		}
		midiData = SciSpan<const byte>(defaultSci32GMPatchMidiData, sizeof(defaultSci32GMPatchMidiData));
	} else {
		error("No MT-32 patch found");
	}

	const bool isEmulatedMt32 = (MidiDriver::getDeviceString(dev, MidiDriver::kDriverId) == "mt32");

	sendBytes(midiData, isEmulatedMt32);
	setMasterVolume(12);
}

Sci1GeneralMidiDriver::~Sci1GeneralMidiDriver() {
	_device->close();
	// TODO: Send All Notes Off controller message if ScummVM does not clean up
	// for us
}

void Sci1GeneralMidiDriver::noteOn(const uint8 channelNo, uint8 note, uint8 velocity) {
	Channel &channel = _channels[channelNo];
	const uint8 origNote = note;
	if (remapNote(channelNo, note)) {
		velocity = _velocityMaps[channel.velocityMap][velocity];
		channel.enabled = true;
		channel.hw->noteOn(note, velocity);
		debugC(kDebugLevelSound, "On  %2d n %3d -> %3d v %3d", channelNo, origNote, note, velocity);
	} else {
		debugC(kDebugLevelSound, "OX  %2d n %3d        v %3d", channelNo, note, velocity);
	}
}

void Sci1GeneralMidiDriver::noteOff(const uint8 channelNo, uint8 note, uint8 velocity) {
	Channel &channel = _channels[channelNo];
	const uint8 origNote = note;
	if (remapNote(channelNo, note)) {
		channel.hw->noteOff(note, velocity);
		debugC(kDebugLevelSound, "Off %2d n %3d -> %3d v %3d", channelNo, origNote, note, velocity);
	} else {
		debugC(kDebugLevelSound, "OXX %2d n %3d        v %3d", channelNo, note, velocity);
	}
}

bool Sci1GeneralMidiDriver::remapNote(const uint8 channelNo, uint8 &note) const {
	const Channel &channel = _channels[channelNo];
	if (channelNo == kPercussionChannel) {
		if (_percussionMap[note] == kUnmapped) {
			return false;
		}
		note = _percussionMap[note];
		return true;
	} else if (channel.outProgram != kUnmapped) {
		note += channel.noteShift;
		const int8 octave = channel.noteShift > 0 ? -kNotesPerOctave : kNotesPerOctave;
		while (note >= kNumNotes) {
			note += octave;
		}
		return true;
	}
	return false;
}

void Sci1GeneralMidiDriver::controllerChange(const uint8 channelNo, const uint8 controllerNo, uint8 value) {
	Channel &channel = _channels[channelNo];
	switch (controllerNo) {
	case kVolumeController: {
		channel.volume = value;
		if (!_isEnabled) {
			return;
		}
		value += channel.volumeShift;
		if (value > kMaxVolume) {
			value = channel.volumeShift > 0 ? kMaxVolume : 1;
		}
		const uint16 result = value * _masterVolume / kMaxMasterVolume;
		value = result & 0xFF;
		if (!value || (result & 0xFF00)) {
			++value;
		}
		break;
	}
	case kPanController:
		if (channel.pan == value) {
			return;
		}
		channel.pan = value;
		break;
	case kDamperPedalController:
		if (channel.damperPedalOn == (value != 0)) {
			return;
		}
		channel.damperPedalOn = value;
		break;
	case kAllNotesOffController:
		if (!channel.enabled) {
			return;
		}
		channel.enabled = false;
		break;
	default:
		return;
	}

	channel.hw->controlChange(controllerNo, value);
	debugC(kDebugLevelSound, "CC %2d %3d %3d", channelNo, controllerNo, value);
}

void Sci1GeneralMidiDriver::programChange(const uint8 channelNo, const uint8 programNo) {
	Channel &channel = _channels[channelNo];
	if (channelNo == kPercussionChannel || channel.program == programNo) {
		return;
	}

	channel.program = programNo;
	channel.velocityMap = _programVelocityMap[programNo];

	bool needsControllerUpdate = channel.outProgram != kUnmapped;
	channel.outProgram = _programMap[programNo];

	if (channel.outProgram == kUnmapped) {
		channel.hw->controlChange(kAllNotesOffController, 0);
		if (_version >= SCI_VERSION_2) {
			channel.hw->controlChange(kDamperPedalController, 0);
		}
		return;
	}

	if (channel.noteShift != _noteShift[programNo]) {
		channel.noteShift = _noteShift[programNo];
		channel.hw->controlChange(kAllNotesOffController, 0);
		if (_version >= SCI_VERSION_2) {
			channel.hw->controlChange(kDamperPedalController, 0);
		}
		needsControllerUpdate = true;
	}

	if (needsControllerUpdate || channel.volumeShift != _volumeShift[programNo]) {
		channel.volumeShift = _volumeShift[programNo];
		controllerChange(channelNo, kVolumeController, channel.volume);
	}

	if (needsControllerUpdate && _version < SCI_VERSION_2) {
		channel.hw->controlChange(kPanController, channel.pan);
		channel.hw->controlChange(kPitchBend, channel.pitchBend - 0x2000);
	}

	channel.hw->programChange(channel.outProgram);
	debugC(kDebugLevelSound, "PC %2d %3d -> %3d", channelNo, programNo, channel.outProgram);
}

void Sci1GeneralMidiDriver::pitchBend(const uint8 channelNo, const uint16 bend) {
	Channel &channel = _channels[channelNo];
	if (channel.pitchBend != bend) {
		channel.pitchBend = bend;
		channel.hw->pitchBend(bend - 0x2000);
		debugC(kDebugLevelSound, "PB %2d %04x", channelNo, bend);
	}
}

void Sci1GeneralMidiDriver::setMasterVolume(const uint8 volume) {
	_masterVolume = volume;

	if (!_isEnabled) {
		return;
	}

	debugC(kDebugLevelSound, "MV %2d", volume);
	for (int i = kMinChannel; i <= kPercussionChannel; ++i) {
		const uint8 channelVolume = _channels[i].volume;
		if (channelVolume != kUnmapped) {
			controllerChange(i, kVolumeController, channelVolume);
		}
	}
}

void Sci1GeneralMidiDriver::enable(const bool enabled) {
	debugC(kDebugLevelSound, "EN %d", enabled);
	Sci1SoundDriver::enable(enabled);
	if (enabled) {
		setMasterVolume(_masterVolume);
	} else {
		uint8 maxChannel = kPercussionChannel;
		if (_version < SCI_VERSION_2) {
			--maxChannel;
		}
		for (int i = kMinChannel; i <= maxChannel; ++i) {
			_channels[i].hw->controlChange(kVolumeController, 0);
		}
	}
}

void Sci1GeneralMidiDriver::debugPrintState(Console &con) const {
	con.debugPrintf("Channels:\n\n");
	for (int i = 0; i < kNumChannels; ++i) {
		const Channel &channel = _channels[i];
		if (channel.program != kUnmapped) {
			con.debugPrintf("%2d: prog %u -> %u bend %04x mod %u pan %u vol %u dp %d\n",
							i,
							channel.program,
							channel.outProgram,
							channel.pitchBend,
							channel.modulation,
							channel.pan,
							channel.volume,
							channel.damperPedalOn);
			con.debugPrintf("    ns %d vs %d vmap %d%s\n",
							channel.noteShift,
							channel.volumeShift,
							channel.velocityMap,
							channel.enabled ? "" : ", disabled");
		} else {
			con.debugPrintf("%2d: unmapped\n", i);
		}
	}
}

void Sci1GeneralMidiDriver::sendBytes(SciSpan<const byte> data, const bool skipDelays) const {
	byte command = 0;

	uint i = 0;
	while (i < data.size()) {
		byte op1, op2;

		if (data[i] & 0x80)
			command = data[i++];

		switch (command & 0xf0) {
		case 0xf0: {
			const byte *sysExStart = data.getUnsafeDataAt(i, data.size() - i);
			const byte *sysExEnd = (const byte *)memchr(sysExStart, 0xf7, data.size() - i);

			if (!sysExEnd)
				error("Failed to find end of sysEx");

			int len = sysExEnd - sysExStart;

			_device->sysEx(sysExStart, len);

			if (!skipDelays) {
				// Wait the time it takes to send the SysEx data
				uint32 delay = (len + 2) * 1000 / 3125;
				g_system->updateScreen();
				g_sci->sleep(delay);
			}

			i += len + 1; // One more for the 0xf7
			break;
		}
		case 0x80:
		case 0x90:
		case 0xa0:
		case 0xb0:
		case 0xe0:
			op1 = data[i++];
			op2 = data[i++];
			_device->send(command, op1, op2);
			break;
		case 0xc0:
		case 0xd0:
			op1 = data[i++];
			_device->send(command, op1, 0);
			break;
		default:
			error("Failed to find MIDI command byte");
		}
	}
}

SoundDriver *makeGeneralMidiDriver(ResourceManager &resMan, const SciVersion version, const bool isMt32) {
	if (version <= SCI_VERSION_01) {
		return nullptr;
	} else {
		return new Sci1GeneralMidiDriver(resMan, version, isMt32);
	}
}

} // End of namespace Sci
