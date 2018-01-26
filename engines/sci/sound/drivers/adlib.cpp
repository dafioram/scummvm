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

#include "audio/fmopl.h"
#include "audio/mididrv.h"
#include "sci/sound/drivers/adlib.h"

namespace Sci {

static const uint8 voiceToOperatorMap[][2] = {
	{ 0,  3 }, {  1,  4 }, {  2,  5 }, {  6,  9 }, { 7, 10 },
	{ 8, 11 }, { 12, 15 }, { 13, 16 }, { 14, 17 }
};

static const uint8 operatorToRegisterMap[] = {
	 0,  1,  2,  3,  4,  5,
	 8,  9, 10, 11, 12, 13,
	16, 17, 18, 19, 20, 21
};

static const uint8 operatorToVoiceMap[] = {
	0, 1, 2, 0, 1, 2,
	3, 4, 5, 3, 4, 5,
	6, 7, 8, 6, 7, 8
};

static const uint16 frequencyNumbers[] = {
	343, 348, 353, 358, 363, 369, 374, 379, 385, 390, 396, 402,
	408, 414, 420, 426, 432, 438, 445, 451, 458, 464, 471, 478,
	485, 492, 499, 506, 514, 521, 529, 536, 544, 552, 560, 568,
	577, 585, 594, 602, 611, 620, 629, 638, 647, 656, 666, 676
};

static const uint8 velocityToOutputLevelMap[] = {
	 0, 12, 13, 14, 15, 17, 18, 19, 20, 22, 23, 24, 26, 27, 28, 29,
	31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 45, 45, 46,
	47, 48, 49, 50, 50, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57, 58,
	59, 59, 59, 60, 60, 60, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63
};

static const uint8 panToOutputLevelMap[] = {
	 0, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 47, 48,
	49, 50, 50, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57, 57, 58, 58,
	59, 59, 59, 60, 60, 60, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63
};

AdLibDriver::Operator AdLibDriver::_defaultProgram[2] = {
	{ 1, 1, 3, 15, 5, 0, 1, 3, 15, 0, 0, 0, 1, 0 },
	{ 0, 1, 1, 15, 7, 0, 2, 4,  0, 0, 0, 1, 0, 0 }
};

AdLibDriver::AdLibDriver(ResourceManager &resMan, SciVersion version) :
	SoundDriver(resMan, version),
	_channels(),
	_voices(),
	_isStereo(true) {

	for (uint i = 0; i < _lruVoice.size(); ++i) {
		_lruVoice[i] = i;
	}

	Resource *patchData = resMan.findResource(ResourceId(kResourceTypePatch, 3), false);
	if (!patchData) {
		// TODO: SCI0early drivers
		error("Could not find AdLib patch data");
	}

	// TODO: Earlier, smaller patches
	SciSpan<const byte> data(*patchData);
	for (uint i = 0; i < _programs.size(); ++i) {
		Program &program = _programs[i];
		for (uint j = 0; j < program.size(); ++j) {
			Operator &op = program[j];
			op.keyScaleLevel = *data++;
			op.frequencyMultiplicationFactor = *data++;
			op.feedbackFactor = *data++;
			op.attackRate = *data++;
			op.sustainLevel = *data++;
			op.sustainOn = *data++;
			op.decayRate = *data++;
			op.releaseRate = *data++;
			op.outputLevel = *data++;
			op.tremoloOn = *data++;
			op.vibratoOn = *data++;
			op.envelopeScalingOn = *data++;
			op.isFrequencyModulation = *data++;
		}
		program[0].waveform = *data++;
		program[1].waveform = *data++;
	}
	data.subspan(0, _rhythmMap.size()).unsafeCopyDataTo(_rhythmMap.data());

	_opl.reset(OPL::Config::create(OPL::Config::kDualOpl2));
	if (!_opl) {
		_isStereo = false;
		_opl.reset(OPL::Config::create(OPL::Config::kOpl2));
	}
	if (!_opl) {
		error("Could not create OPL");
	}
	if (!_opl->init()) {
		error("Could not init OPL");
	}

	_opl->start(nullptr);

	resetOpl();
}

AdLibDriver::~AdLibDriver() {
	resetOpl();
	_opl->stop();
}

void AdLibDriver::service() {
	// no-op in SCI32
}

void AdLibDriver::noteOn(const uint8 channelIndex, const uint8 note, uint8 velocity) {
	if (note < 12 || note > 107) {
		return;
	}

	velocity /= 2;

	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voice.currentChannel == channelIndex && voice.note == note) {
			voiceOff(i);
			voiceOn(i, note, velocity);
			return;
		}
	}

	uint8 voiceNo = findFreeVoice(channelIndex);
	if (voiceNo != kUnmapped) {
		voiceOn(voiceNo, note, velocity);
	}
}

void AdLibDriver::noteOff(const uint8 channelIndex, const uint8 note, const uint8 velocity) {
	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voice.originalChannel == channelIndex && voice.note == note) {
			if (_channels[channelIndex].damperPedalOn) {
				voice.damperPedalOn = true;
			} else {
				voiceOff(i);
			}
		}
	}
}

void AdLibDriver::controllerChange(const uint8 channelIndex, const uint8 controllerNo, const uint8 value) {
	Channel &channel = _channels[channelIndex];

	bool voiceOn;

	switch (controllerNo) {
	case kDamperPedalController:
		channel.damperPedalOn = (value != 0);
		if (!value) {
			for (uint i = 0; i < _voices.size(); ++i) {
				Voice &voice = _voices[i];
				if (voice.originalChannel == channelIndex && voice.damperPedalOn) {
					voiceOff(i);
				}
			}
		}
		return;

	case kMaxVoicesController:
		setChannelNumVoices(channelIndex, value);
		return;

	default:
		return;

	case kVolumeController:
		channel.volume = value / 2;
		voiceOn = true;
		break;

	case kPanController:
		channel.pan = value;
		voiceOn = true;
		break;

	case kAllNotesOffController:
		voiceOn = false;
		break;
	}

	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voice.originalChannel == channelIndex && voice.note != kUnmapped) {
			if (voiceOn) {
				sendNote(i, true);
			} else {
				voiceOff(i);
			}
		}
	}
}

void AdLibDriver::setChannelNumVoices(const uint8 channelIndex, const uint8 numVoices) {
	uint8 numActiveVoices = 0;

	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voice.currentChannel == channelIndex) {
			++numActiveVoices;
		}
	}

	numActiveVoices += _channels[channelIndex].numReservedVoices;
	if (numActiveVoices > numVoices) {
		releaseVoices(channelIndex, numActiveVoices - numVoices);
	} else if (numActiveVoices < numVoices) {
		assignVoices(channelIndex, numVoices - numActiveVoices);
	}
}

void AdLibDriver::programChange(const uint8 channelIndex, const uint8 programNo) {
	_channels[channelIndex].program = programNo;
}

void AdLibDriver::pitchBend(const uint8 channelIndex, const uint16 bend) {
	_channels[channelIndex].pitchBend = bend;
	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voice.originalChannel == channelIndex && voice.note != kUnmapped) {
			sendNote(i, true);
		}
	}
}

void AdLibDriver::enable(const bool enable) {
	SoundDriver::enable(enable);
	const uint8 masterVolume = _masterVolume;
	if (enable) {
		setMasterVolume(masterVolume);
	} else {
		setMasterVolume(0);
		_masterVolume = masterVolume;
	}
}

void AdLibDriver::setMasterVolume(const uint8 volume) {
	SoundDriver::setMasterVolume(volume);
	for (uint i = 0; i < _voices.size(); ++i) {
		if (_voices[i].note != kUnmapped) {
			sendNote(i, true);
		}
	}
}

void AdLibDriver::sendToHardware(const uint8 registerNo, const uint8 value) {
	sendRight(registerNo, value);
	sendLeft(registerNo, value);
}

void AdLibDriver::sendLeft(const uint8 registerNo, uint8 value) {
	if (!_isStereo) {
		return;
	}

	if (registerNo >= kSpeakerEnableRegisterLow && registerNo <= kSpeakerEnableRegisterHigh) {
		enum { kEnableLeftSpeaker = 0x10 };
		value |= kEnableLeftSpeaker;
	}

	_opl->write(0x222, registerNo);
	_opl->write(0x223, value);
}

void AdLibDriver::sendRight(const uint8 registerNo, uint8 value) {
	if (registerNo >= kSpeakerEnableRegisterLow && registerNo <= kSpeakerEnableRegisterHigh) {
		enum { kEnableRightSpeaker = 0x20 };
		value |= kEnableRightSpeaker;
	}

	_opl->write(0x220, registerNo);
	_opl->write(0x221, value);
}

void AdLibDriver::resetOpl() {
	enum { kMaxRegister = 0xf5 };
	for (uint reg = 0; reg <= kMaxRegister; ++reg) {
		if (reg == 0 || reg == 5 || reg == 6 || reg == 7 || (reg >= 9 && reg <= 0x1f)) {
			continue;
		}
		sendToHardware(reg, 0);
	}

	enum { kEnable = 0x20 };
	sendToHardware(kWaveformSelectEnableRegister, kEnable);

	for (uint op = 0; op < kNumOperators; ++op) {
		sendOperator(op, _defaultProgram[op % 6 / 3]);
	}
}

void AdLibDriver::assignVoices(const uint8 channelIndex, uint8 numVoices) {
	Channel &channel = _channels[channelIndex];
	for (uint i = 0; i < _voices.size() && numVoices; ++i) {
		Voice &voice = _voices[i];
		if (voice.currentChannel == kUnmapped) {
			if (voice.note != kUnmapped) {
				voiceOff(i);
			}
			voice.currentChannel = channelIndex;
			++channel.numAssignedVoices;
			--numVoices;
		}
	}
	channel.numReservedVoices += numVoices;
}

void AdLibDriver::releaseVoices(const uint8 channelIndex, uint8 numVoices) {
	Channel &channel = _channels[channelIndex];
	if (channel.numReservedVoices >= numVoices) {
		channel.numReservedVoices -= numVoices;
		return;
	}

	numVoices -= channel.numReservedVoices;
	channel.numReservedVoices = 0;

	for (uint i = 0; i < _voices.size() && numVoices; ++i) {
		Voice &voice = _voices[i];
		if (voice.currentChannel == channelIndex && voice.note == kUnmapped) {
			voice.currentChannel = kUnmapped;
			--channel.numAssignedVoices;
			--numVoices;
		}
	}

	for (uint i = 0; i < _voices.size() && numVoices; ++i) {
		Voice &voice = _voices[i];
		if (voice.currentChannel == channelIndex) {
			voiceOff(i);
			voice.currentChannel = kUnmapped;
			--channel.numAssignedVoices;
			--numVoices;
		}
	}

	// This is DistributeVoices
	numVoices = 0;
	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voice.currentChannel == kUnmapped) {
			++numVoices;
		}
	}

	for (uint i = 0; i < _channels.size(); ++i) {
		Channel &newChannel = _channels[i];
		if (newChannel.numReservedVoices) {
			if (newChannel.numReservedVoices >= numVoices) {
				newChannel.numReservedVoices -= numVoices;
				assignVoices(i, numVoices);
			} else {
				const uint8 voicesToAssign = newChannel.numReservedVoices;
				numVoices -= voicesToAssign;
				newChannel.numReservedVoices = 0;
				assignVoices(i, voicesToAssign);
			}
		}
	}
}

void AdLibDriver::sendOperator(const uint8 opNo, const Operator &op) {
	sendToHardware(kPercussonRegister, 0);
	sendToHardware(kKeySplitRegister, 0);

	const uint8 voiceNo = operatorToVoiceMap[opNo];
	const uint8 registerNo = operatorToRegisterMap[opNo];

	uint8 value = op.keyScaleLevel << 6 | (op.outputLevel & 0x3f);
	sendToHardware(kKeyScaleOutputLevelRegister + registerNo, value);

	value = (op.feedbackFactor << 1 | (op.isFrequencyModulation ? 0 : 1)) & 0xf;
	sendToHardware(kSpeakerEnableRegisterLow + voiceNo, value);

	value = op.attackRate << 4 | (op.decayRate & 0xf);
	sendToHardware(kAttackDecayRegister + registerNo, value);

	value = op.sustainLevel << 4 | (op.releaseRate & 0xf);
	sendToHardware(kSustainReleaseRegister + registerNo, value);

	value = 0;
	if (op.tremoloOn) {
		value |= 0x80;
	}
	if (op.vibratoOn) {
		value |= 0x40;
	}
	if (op.sustainOn) {
		value |= 0x20;
	}
	if (op.envelopeScalingOn) {
		value |= 0x10;
	}
	value |= op.frequencyMultiplicationFactor & 0xf;
	sendToHardware(kSavekRegister + registerNo, value);

	sendToHardware(kWaveformSelectRegister + registerNo, op.waveform);
}

uint8 AdLibDriver::findFreeVoice(const uint8 channelNo) {
	uint8 nextBestVoiceNo = kUnmapped;
	for (uint i = 0; i < _voices.size(); ++i) {
		const uint8 voiceNo = _lruVoice[i];
		Voice &voice = _voices[voiceNo];
		if (voice.note == kUnmapped) {
			if (voice.program == _channels[channelNo].program) {
				voice.originalChannel = channelNo;
				return voiceNo;
			// SSCI ran a second loop without the program number check if it did
			// not find a voice; we just do one loop and store the would-be
			// result of the second loop at the same time instead
			} else if (nextBestVoiceNo == kUnmapped) {
				nextBestVoiceNo = voiceNo;
			}
		}
	}

	if (nextBestVoiceNo != kUnmapped) {
		_voices[nextBestVoiceNo].originalChannel = channelNo;
		return nextBestVoiceNo;
	}

	uint8 bestChannelNo = 0;
	uint8 bestVoicesDelta = 0;
	for (uint i = 0; i < _channels.size(); ++i) {
		Channel &channel = _channels[i];
		uint8 numActiveVoices = channel.numActiveVoices;
		if (numActiveVoices > channel.numAssignedVoices) {
			numActiveVoices -= channel.numAssignedVoices;
			if (bestVoicesDelta < numActiveVoices) {
				bestVoicesDelta = numActiveVoices;
				bestChannelNo = i;
			}
		}
	}

	if (!bestVoicesDelta) {
		bestChannelNo = channelNo;
	}

	for (uint i = 0; i < _voices.size(); ++i) {
		const uint8 voiceNo = _lruVoice[i];
		Voice &voice = _voices[voiceNo];
		if (voice.originalChannel == bestChannelNo) {
			voiceOff(voiceNo);
			voice.originalChannel = channelNo;
			return voiceNo;
		}
	}

	return kUnmapped;
}

void AdLibDriver::voiceOn(const uint8 voiceNo, const uint8 note, const uint8 velocity) {
	Voice &voice = _voices[voiceNo];
	Channel &channel = _channels[voice.originalChannel];
	++channel.numActiveVoices;
	updateLru(voiceNo);

	uint8 programNo;
	if (voice.originalChannel == kPercussionChannel) {
		programNo = CLIP<uint8>(note, 27, 88) + 101;
	} else {
		programNo = channel.program;
	}

	if (voice.program != programNo && isEnabled()) {
		voice.program = programNo;
		setVoiceProgram(voiceNo, programNo);
	}

	voice.velocity = velocity;
	voice.note = note;
	sendNote(voiceNo, true);
}

void AdLibDriver::voiceOff(const uint8 voiceNo) {
	Voice &voice = _voices[voiceNo];
	voice.damperPedalOn = false;
	sendNote(voiceNo, false);
	voice.note = kUnmapped;
	updateLru(voiceNo);
	--_channels[voice.originalChannel].numActiveVoices;
}

void AdLibDriver::updateLru(const uint8 voiceNo) {
	for (uint i = 0; i < _lruVoice.size(); ++i) {
		if (_lruVoice[i] == voiceNo) {
			for (; i < _lruVoice.size() - 1; ++i) {
				_lruVoice[i] = _lruVoice[i + 1];
			}
			break;
		}
	}
	_lruVoice.back() = voiceNo;
}

void AdLibDriver::setVoiceProgram(const uint8 voiceNo, const uint8 programNo) {
	Voice &voice = _voices[voiceNo];
	Program &program = _programs[programNo];
	voice.isAdditive = !program[0].isFrequencyModulation;

	if (voice.isAdditive) {
		voice.operators[0].keyScaleLevel = program[0].keyScaleLevel;
		voice.operators[0].outputLevel = 63 - program[0].outputLevel;
	}

	voice.operators[1].keyScaleLevel = program[1].keyScaleLevel;
	voice.operators[1].outputLevel = 63 - program[1].outputLevel;

	// SSCI used an extra indirection here because of the way data in the patch
	// file is arranged; we preprocess the patch data so can call sendOperator
	// directly
	sendOperator(voiceToOperatorMap[voiceNo][0], program[0]);
	sendOperator(voiceToOperatorMap[voiceNo][1], program[1]);
}

void AdLibDriver::sendNote(const uint8 voiceNo, const bool noteOn) {
	Voice &voice = _voices[voiceNo];
	// SSCI assigned the note at the start of this function; we do it in the
	// caller instead since almost every caller was just sending the note which
	// was already assigned to the voice
	uint8 note;
	Channel &channel = _channels[voice.originalChannel];
	if (voice.program >= 128) {
		note = _rhythmMap[CLIP<uint8>(voice.note, 27, 88) - 27];
	} else {
		note = voice.note;
	}

	// SSCI called a separate function and then checked for a return value of
	// 0xffff, but no such value could ever be generated by that function and
	// it was only ever called once, so its calculation is inlined here
	const uint16 frequency = MIN(508, note * 4 + (channel.pitchBend - 0x2000) / 171);
	const uint8 fNumberIndex = frequency % ARRAYSIZE(frequencyNumbers);
	uint8 fBlockNumber = frequency / ARRAYSIZE(frequencyNumbers);
	if (fBlockNumber) {
		--fBlockNumber;
	}
	const uint16 frequencyNumber = frequencyNumbers[fNumberIndex];

	assert(voice.velocity < ARRAYSIZE(velocityToOutputLevelMap));
	sendVolume(voiceNo, (channel.volume + 1) * (velocityToOutputLevelMap[voice.velocity] + 1) / (kMaxVolume + 1) * _masterVolume / kMaxMasterVolume);

	uint8 value = fBlockNumber << 2; // block number
	value |= (frequencyNumber >> 8); // frequency number high bits
	if (noteOn) {
		enum { kNoteOn = 0x20 };
		value |= kNoteOn;
	}

	sendToHardware(kLowFrequencyNumberRegister + voiceNo, frequencyNumber);
	sendToHardware(kHighFrequencyNumberRegister + voiceNo, value);
}

void AdLibDriver::sendVolume(const uint8 voiceNo, uint8 volume) {
	if (volume > kMaxVolume) {
		volume = kMaxVolume;
	}

	Voice &voice = _voices[voiceNo];
	Channel &channel = _channels[voice.originalChannel];

	for (uint stereoChannel = 0; stereoChannel < 1 + _isStereo; ++stereoChannel) {
		uint8 panVolume;
		if (!_isStereo) {
			panVolume = kMaxVolume;
		} else if (stereoChannel == 0) {
			panVolume = channel.pan / 2;
		} else {
			panVolume = (kMaxPan - channel.pan) / 2;
		}
		panVolume = MIN<uint8>(panVolume, kMaxVolume);

		const uint8 pannedVolume = panToOutputLevelMap[MIN<uint8>(volume - (volume * (kMaxVolume - panToOutputLevelMap[panVolume]) / kMaxVolume), kMaxVolume)];

		uint8 operatorVolume = kMaxVolume - pannedVolume * voice.operators[1].outputLevel / kMaxVolume;
		uint8 value = voice.operators[1].keyScaleLevel << 6 | operatorVolume;
		uint8 reg = kKeyScaleOutputLevelRegister + voiceToOperatorMap[voiceNo][1];

		if (stereoChannel == 0) {
			sendRight(reg, value);
		} else {
			sendLeft(reg, value);
		}

		if (voice.isAdditive) {
			operatorVolume = kMaxVolume - pannedVolume * voice.operators[0].outputLevel / kMaxVolume;
			value = voice.operators[0].keyScaleLevel << 6 | operatorVolume;
			reg = kKeyScaleOutputLevelRegister + voiceToOperatorMap[voiceNo][0];
			if (stereoChannel == 0) {
				sendRight(reg, value);
			} else {
				sendLeft(reg, value);
			}
		}
	}
}

void AdLibDriver::debugPrintState(Console &con) const {
	con.debugPrintf("Voices:\n");
	for (uint i = 0; i < _voices.size(); ++i) {
		const Voice &voice = _voices[i];

		if (voice.originalChannel == kUnmapped && voice.currentChannel == kUnmapped) {
			con.debugPrintf("%d: unmapped\n", i);
			continue;
		}

		con.debugPrintf("%d: ch %2d / %2d pr %3d v %2d n %3d dp %d\n",
						i,
						voice.originalChannel, voice.currentChannel,
						voice.program, voice.velocity, voice.note,
						voice.damperPedalOn);

		if (voice.isAdditive) {
			con.debugPrintf("   op 1: ksl %d ol %d\n",
							voice.operators[0].keyScaleLevel,
							voice.operators[0].outputLevel);
		} else {
			con.debugPrintf("   op 1: N/A\n");
		}

		con.debugPrintf("   op 2: ksl %d ol %d\n",
						voice.operators[1].keyScaleLevel,
						voice.operators[1].outputLevel);
	}

	con.debugPrintf("\nChannels:\n");
	for (uint i = 0; i < _channels.size(); ++i) {
		const Channel &channel = _channels[i];

		if (!channel.program) {
			con.debugPrintf("%2d: unmapped\n", i);
			continue;
		}

		con.debugPrintf("%2d: pr %3d v %2d p %2d pb %04x dp %d\n",
						i, channel.program, channel.volume, channel.pan,
						channel.pitchBend, channel.damperPedalOn);
		con.debugPrintf("    res.v %d ass.v %d act.v %d\n",
						channel.numReservedVoices, channel.numAssignedVoices,
						channel.numActiveVoices);
	}
}


SoundDriver *makeAdLibDriver(ResourceManager &resMan, SciVersion version) {
	return new AdLibDriver(resMan, version);
}

} // End of namespace Sci
