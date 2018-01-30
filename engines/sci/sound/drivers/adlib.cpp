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

static const uint8 velocityMap1[] = {
	 0, 12, 13, 14, 15, 17, 18, 19, 20, 22, 23, 24, 26, 27, 28, 29,
	31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 45, 45, 46,
	47, 48, 49, 50, 50, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57, 58,
	59, 59, 59, 60, 60, 60, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63
};

static const uint8 velocityMap2[] = {
	 0, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 47, 48,
	49, 50, 50, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57, 57, 58, 58,
	59, 59, 59, 60, 60, 60, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63
};

const AdLibDriver::Operator AdLibDriver::_defaultOperators[2] = {
	{ 1, 1, 3, 15, 5, false, 1, 3, 15, false, false, false,  true, 0 },
	{ 0, 1, 1, 15, 7, false, 2, 4,  0, false, false,  true, false, 0 }
};

AdLibDriver::AdLibDriver(ResourceManager &resMan, SciVersion version) :
	SoundDriver(resMan, version),
	_channels(),
	_voices(),
	_isStereo(false) {

	for (uint i = 0; i < _lruVoice.size(); ++i) {
		_lruVoice[i] = i;
	}

	Resource *patchData = resMan.findResource(ResourceId(kResourceTypePatch, 3), false);
	if (!patchData) {
		// TODO: SCI0early drivers
		error("Could not find AdLib patch data");
	}

	SciSpan<const byte> data(*patchData);
	if (data.size() > 2690) {
		for (uint i = 0; i < _programs.size(); ++i) {
			readProgramFromPatch(data, _programs[i]);
		}
		data.subspan(0, _rhythmMap.size()).unsafeCopyDataTo(_rhythmMap.data());
	} else {
		// The different patch sizes change not only the patch data, but also
		// the behaviour of the driver itself, even though this is not tied to
		// any particular sound version (e.g. SQ4 uses "SCI1late" type driver
		// even though the rest of its sound system is SCI1.1)
		if (_version > SCI_VERSION_1_LATE) {
			_version = SCI_VERSION_1_LATE;
		}

		uint i = 0;
		for (; i < 48; ++i) {
			readProgramFromPatch(data, _programs[i]);
		}
		if (data.size() > 2 && data.getUint16BEAt(0) == 0xabcd) {
			data += 2;
			for (; i < 96; ++i) {
				readProgramFromPatch(data, _programs[i]);
			}
		}
		for (; i < _programs.size(); ++i) {
			_programs[i] = {};
		}
	}

	if (_version <= SCI_VERSION_01) {
		for (uint i = 0; i < _channels.size(); ++i) {
			_channels[i].program = 13;
		}
	}

	// TODO: This is a clumsy way of sending OPL resets in the same way that
	// they are sent in SSCI
	bool isStereo = false;

	// TODO: It might be better to just always use dual-OPL2 since pre-SCI1.1
	// did not support OPL3 (or dual-OPL2) at all, so the
	// not-quite-OPL2-compatible mode might not be the best choice if we decide
	// to allow stereo support in these older games
	if (_version >= SCI_VERSION_1_EARLY) {
		_opl.reset(OPL::Config::create(OPL::Config::kOpl3));
		isStereo = true;
	}

	if (!_opl) {
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

	if (isStereo) {
		_isStereo = true;
		sendLeft(kEnableOpl3Register, 1);
	}

	setMasterVolume(15);
}

AdLibDriver::~AdLibDriver() {
	resetOpl();
	_opl->stop();
}

void AdLibDriver::service() {
	// no-op

	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voice.note != kUnmapped) {
			++voice.numActiveTicks;
		}
	}
}

void AdLibDriver::noteOn(const uint8 channelNo, const uint8 note, uint8 velocity) {
	if (note < 12 || note > 107) {
		return;
	}

	velocity /= 2;

	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voice.originalChannel == channelNo && voice.note == note) {
			if (_version < SCI_VERSION_1_1) {
				voice.damperPedalOn = false;
			}
			voiceOff(i);
			voiceOn(i, note, velocity);
			return;
		}
	}

	uint8 voiceNo = findFreeVoice(channelNo);
	if (voiceNo != kUnmapped) {
		debugC(kDebugLevelSound, "On  %2d -> %d n %3d v %3d", channelNo, voiceNo, note, velocity);
		voiceOn(voiceNo, note, velocity);
	} else {
		debugC(kDebugLevelSound, "OX  %2d      n %3d v %3d", channelNo, note, velocity);
	}
}

void AdLibDriver::noteOff(const uint8 channelNo, const uint8 note, const uint8 velocity) {
	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voice.originalChannel == channelNo && voice.note == note) {
			if (_channels[channelNo].damperPedalOn) {
				voice.damperPedalOn = true;
			} else {
				debugC(kDebugLevelSound, "Off %2d -> %d n %3d v %3d", channelNo, i, note, velocity);
				voiceOff(i);
			}
		}
	}
}

void AdLibDriver::controllerChange(const uint8 channelNo, const uint8 controllerNo, const uint8 value) {
	Channel &channel = _channels[channelNo];
	debugC(kDebugLevelSound, "CC %2d %3d %3d", channelNo, controllerNo, value);

	bool voiceOn;

	switch (controllerNo) {
	case kDamperPedalController:
		channel.damperPedalOn = (value != 0);
		if (!value) {
			for (uint i = 0; i < _voices.size(); ++i) {
				Voice &voice = _voices[i];
				if (voice.originalChannel == channelNo && voice.damperPedalOn) {
					if (_version < SCI_VERSION_1_1) {
						voice.damperPedalOn = false;
					}
					voiceOff(i);
				}
			}
		}
		return;

	case kMaxVoicesController:
		setChannelExtraVoices(channelNo, value);
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
		if (voice.originalChannel == channelNo && voice.note != kUnmapped) {
			if (voiceOn) {
				sendNote(i, true);
			} else {
				voiceOff(i);
			}
		}
	}
}

void AdLibDriver::programChange(const uint8 channelNo, const uint8 programNo) {
	_channels[channelNo].program = programNo;
	debugC(kDebugLevelSound, "PC %2d %3d", channelNo, programNo);

	if (_version <= SCI_VERSION_01) {
		for (uint i = 0; i < _voices.size(); ++i) {
			Voice &voice = _voices[i];
			if (voice.originalChannel != kUnmapped) {
				setVoiceProgram(i, programNo);
				setVoiceVolume(i);
			}
		}
	}
}

void AdLibDriver::pitchBend(const uint8 channelNo, const uint16 bend) {
	_channels[channelNo].pitchBend = bend;
	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		// SSCI1late- did not check for unmapped note, but would just end up
		// aborting in sendNote since kUnmapped is above the maximum note range
		if (voice.originalChannel == channelNo && voice.note != kUnmapped) {
			sendNote(i, true);
			debugC(kDebugLevelSound, "PB %2d -> %d %04x", channelNo, i, bend);
		}
	}
}

void AdLibDriver::setMasterVolume(const uint8 volume) {
	debugC(kDebugLevelSound, "MV %2d", volume);
	SoundDriver::setMasterVolume(volume);
	for (uint i = 0; i < _voices.size(); ++i) {
		if (_voices[i].note != kUnmapped) {
			sendNote(i, true);
		}
	}
}

void AdLibDriver::enable(const bool enabled) {
	debugC(kDebugLevelSound, "EN %d", enabled);
	SoundDriver::enable(enabled);
	if (_version >= SCI_VERSION_1_1) {
		const uint8 masterVolume = _masterVolume;
		if (enabled) {
			setMasterVolume(masterVolume);
		} else {
			setMasterVolume(0);
			_masterVolume = masterVolume;
		}
	}
}

void AdLibDriver::debugPrintState(Console &con) const {
	con.debugPrintf("Voices:\n");
	for (uint i = 0; i < _voices.size(); ++i) {
		const Voice &voice = _voices[i];

		if (voice.originalChannel == kUnmapped && voice.extraChannel == kUnmapped) {
			con.debugPrintf("%d: unmapped\n", i);
			continue;
		}

		con.debugPrintf("%d: ch %2d / %2d pr %3d v %2d n %3d dp %d\n",
						i,
						voice.originalChannel, voice.extraChannel,
						voice.program, voice.velocity, voice.note,
						voice.damperPedalOn);

		if (voice.program >= _programs.size()) {
			continue;
		}

		for (uint j = 0; j < 2; ++j) {
			if (j == 0 && !voice.isAM) {
				con.debugPrintf("   op 1: N/A\n");
				continue;
			}
			const Operator &op = _programs[voice.program][j];
			con.debugPrintf("   op %d: wf %d ksl %d ol %d adsr %d %d %d %d\n",
							j + 1,
							op.waveform,
							op.keyScaleLevel,
							op.outputLevel,
							op.attackRate,
							op.decayRate,
							op.sustainLevel,
							op.releaseRate);
			con.debugPrintf("         v %d t %d s %d es %d ff %d fmf %d\n",
							op.vibratoOn, op.tremoloOn, op.sustainOn,
							op.envelopeScalingOn, op.feedbackFactor,
							op.frequencyMultiplicationFactor);
		}
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
						channel.numInactiveExtraVoices, channel.numActiveExtraVoices,
						channel.numActiveVoices);
	}
}

void AdLibDriver::readProgramFromPatch(SciSpan<const byte> &data, Program &program) {
	for (uint i = 0; i < program.size(); ++i) {
		Operator &op = program[i];
		op.keyScaleLevel = *data++ & 0x3;
		op.frequencyMultiplicationFactor = *data++ & 0xf;
		op.feedbackFactor = *data++ & 0x7;
		op.attackRate = *data++ & 0xf;
		op.sustainLevel = *data++ & 0xf;
		op.sustainOn = *data++;
		op.decayRate = *data++ & 0xf;
		op.releaseRate = *data++ & 0xf;
		op.outputLevel = *data++ & 0x3f;
		op.tremoloOn = *data++;
		op.vibratoOn = *data++;
		op.envelopeScalingOn = *data++;
		op.isFrequencyModulation = *data++;
	}
	program[0].waveform = *data++ & 0x3;
	program[1].waveform = *data++ & 0x3;
}

void AdLibDriver::setChannelExtraVoices(const uint8 channelNo, const uint8 numVoices) {
	uint8 numActiveVoices = 0;

	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voiceExtraChannel(voice) == channelNo) {
			++numActiveVoices;
		}
	}

	numActiveVoices += _channels[channelNo].numInactiveExtraVoices;
	if (numActiveVoices > numVoices) {
		releaseExtraVoices(channelNo, numActiveVoices - numVoices);
	} else if (numActiveVoices < numVoices) {
		assignExtraVoices(channelNo, numVoices - numActiveVoices);
	}
}

void AdLibDriver::voiceOn(const uint8 voiceNo, const uint8 note, const uint8 velocity) {
	Voice &voice = _voices[voiceNo];
	Channel &channel = _channels[voice.originalChannel];
	++channel.numActiveVoices;
	updateLru(voiceNo);

	uint8 programNo;
	if (_version >= SCI_VERSION_1_1 && voice.originalChannel == kPercussionChannel) {
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
	if (_version >= SCI_VERSION_1_1) {
		voice.damperPedalOn = false;
	} else if (voice.damperPedalOn) {
		return;
	}
	sendNote(voiceNo, false);
	voice.note = kUnmapped;
	updateLru(voiceNo);
	--_channels[voice.originalChannel].numActiveVoices;
}

void AdLibDriver::updateLru(const uint8 voiceNo) {
	if (_version < SCI_VERSION_1_1) {
		_voices[voiceNo].numActiveTicks = 0;
	} else {
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
}

uint8 AdLibDriver::findFreeVoice(const uint8 channelNo) {
	if (_version < SCI_VERSION_1_1) {
		Channel &channel = _channels[channelNo];
		uint8 candidateVoiceNo = channel.lastVoice;
		uint8 foundVoiceNo = kUnmapped;
		uint8 foundVoiceAge = 0;
		for (bool found = false; !found; ) {
			++candidateVoiceNo;
			if (candidateVoiceNo == _voices.size()) {
				candidateVoiceNo = 0;
			}

			Voice &voice = _voices[candidateVoiceNo];

			if (candidateVoiceNo == channel.lastVoice) {
				found = true;
			}

			if (voice.originalChannel == channelNo) {
				if (voice.note == kUnmapped) {
					channel.lastVoice = candidateVoiceNo;
					return candidateVoiceNo;
				} else {
					const uint8 candidateAge = voice.numActiveTicks;
					if (candidateAge >= foundVoiceAge) {
						foundVoiceAge = candidateAge;
						foundVoiceNo = candidateVoiceNo;
					}
				}
			}
		}

		if (foundVoiceAge != 0) {
			_voices[foundVoiceNo].damperPedalOn = false;
			voiceOff(foundVoiceNo);
			channel.lastVoice = foundVoiceNo;
		}

		return foundVoiceNo;
	} else {
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
			if (numActiveVoices > channel.numActiveExtraVoices) {
				numActiveVoices -= channel.numActiveExtraVoices;
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
	}

	return kUnmapped;
}

void AdLibDriver::assignExtraVoices(const uint8 channelNo, uint8 numVoices) {
	Channel &channel = _channels[channelNo];
	for (uint i = 0; i < _voices.size() && numVoices; ++i) {
		Voice &voice = _voices[i];
		if (voiceExtraChannel(voice) == kUnmapped) {
			if (voice.note != kUnmapped) {
				voiceOff(i);
			}
			voiceExtraChannel(voice) = channelNo;
			++channel.numActiveExtraVoices;
			--numVoices;
		}
	}
	channel.numInactiveExtraVoices += numVoices;
}

void AdLibDriver::releaseExtraVoices(const uint8 channelNo, uint8 numVoices) {
	Channel &channel = _channels[channelNo];
	if (channel.numInactiveExtraVoices >= numVoices) {
		channel.numInactiveExtraVoices -= numVoices;
		return;
	}

	numVoices -= channel.numInactiveExtraVoices;
	channel.numInactiveExtraVoices = 0;

	for (uint i = 0; i < _voices.size() && numVoices; ++i) {
		Voice &voice = _voices[i];
		if (voiceExtraChannel(voice) == channelNo && voice.note == kUnmapped) {
			voiceExtraChannel(voice) = kUnmapped;
			--channel.numActiveExtraVoices;
			--numVoices;
		}
	}

	for (uint i = 0; i < _voices.size() && numVoices; ++i) {
		Voice &voice = _voices[i];
		if (voiceExtraChannel(voice) == channelNo) {
			if (_version < SCI_VERSION_1_1) {
				voice.damperPedalOn = false;
			}
			voiceOff(i);
			voiceExtraChannel(voice) = kUnmapped;
			--channel.numActiveExtraVoices;
			--numVoices;
		}
	}

	// In SSCI this code was in a separate function; since it was only invoked
	// once from the same place, we just merge them both together
	numVoices = 0;
	for (uint i = 0; i < _voices.size(); ++i) {
		Voice &voice = _voices[i];
		if (voiceExtraChannel(voice) == kUnmapped) {
			++numVoices;
		}
	}

	for (uint i = 0; i < _channels.size(); ++i) {
		Channel &newChannel = _channels[i];
		if (newChannel.numInactiveExtraVoices) {
			if (newChannel.numInactiveExtraVoices >= numVoices) {
				newChannel.numInactiveExtraVoices -= numVoices;
				assignExtraVoices(i, numVoices);
			} else {
				const uint8 voicesToAssign = newChannel.numInactiveExtraVoices;
				numVoices -= voicesToAssign;
				newChannel.numInactiveExtraVoices = 0;
				assignExtraVoices(i, voicesToAssign);
			}
		}
	}
}

void AdLibDriver::setVoiceProgram(const uint8 voiceNo, const uint8 programNo) {
	Voice &voice = _voices[voiceNo];
	Program &program = _programs[programNo];
	voice.isAM = !program[0].isFrequencyModulation;

	if (voice.isAM) {
		voice.operators[0].keyScaleLevel = program[0].keyScaleLevel;
		voice.operators[0].outputLevel = kMaxVolume - program[0].outputLevel;
	}

	voice.operators[1].keyScaleLevel = program[1].keyScaleLevel;
	voice.operators[1].outputLevel = kMaxVolume - program[1].outputLevel;

	// SSCI did some extra work here, where patch data was processed and
	// assigned to a separate array of operators on every program change; we
	// skip that unnecessary extra work entirely by preprocessing the patch data
	// at driver startup so we can just send the program's operator parameters
	// directly to the hardware
	sendOperator(voiceToOperatorMap[voiceNo][0], program[0]);
	sendOperator(voiceToOperatorMap[voiceNo][1], program[1]);
}

void AdLibDriver::setVoiceVolume(const uint8 voiceNo) {
	Voice &voice = _voices[voiceNo];
	Channel &channel = _channels[voice.originalChannel];

	uint8 volume;
	if (isEnabled()) {
		volume = (channel.volume + 1) * (velocityMap1[voice.velocity] + 1) / (kMaxVolume + 1);
		if (_version < SCI_VERSION_1_1) {
			--volume;
		} else if (volume > 63) {
			volume = 63;
		}
		volume = volume * (_masterVolume + 1) / (kMaxMasterVolume + 1);
	} else {
		volume = 0;
	}

	for (uint stereoChannel = 0; stereoChannel < 1 + _isStereo; ++stereoChannel) {
		uint8 panVolume;
		if (!_isStereo) {
			panVolume = kMaxVolume;
		} else if (stereoChannel == 0) {
			panVolume = channel.pan / 2;
		} else {
			panVolume = (kMaxPan - channel.pan) / 2;
		}

		const uint8 pannedVolume = velocityMap2[CLIP<int>(volume - volume * (kMaxVolume - velocityMap2[panVolume]) / kMaxVolume, 0, kMaxVolume)];

		uint8 operatorVolume = kMaxVolume - pannedVolume * voice.operators[1].outputLevel / kMaxVolume;
		uint8 value = voice.operators[1].keyScaleLevel << 6 | operatorVolume;
		uint8 reg = kKeyScaleOutputLevelRegister + operatorToRegisterMap[voiceToOperatorMap[voiceNo][1]];

		if (!_isStereo) {
			sendToHardware(reg, value);
		} else if (stereoChannel == 0) {
			sendRight(reg, value);
		} else {
			sendLeft(reg, value);
		}

		if (voice.isAM) {
			operatorVolume = kMaxVolume - pannedVolume * voice.operators[0].outputLevel / kMaxVolume;
			value = voice.operators[0].keyScaleLevel << 6 | operatorVolume;
			reg = kKeyScaleOutputLevelRegister + operatorToRegisterMap[voiceToOperatorMap[voiceNo][0]];

			if (!_isStereo) {
				sendToHardware(reg, value);
			} else if (stereoChannel == 0) {
				sendRight(reg, value);
			} else {
				sendLeft(reg, value);
			}
		}
	}
}

void AdLibDriver::sendNote(const uint8 voiceNo, const bool noteOn) {
	Voice &voice = _voices[voiceNo];
	Channel &channel = _channels[voice.originalChannel];

	// SSCI assigned the note at the start of this function; we do it in the
	// caller instead since almost every caller was just sending the note which
	// was already assigned to the voice
	uint8 note;
	if (_version >= SCI_VERSION_1_1 && voice.program >= 128) {
		note = _rhythmMap[CLIP<uint8>(voice.note, 27, 88) - 27];
	} else {
		note = voice.note;
	}

	// SSCI called a separate function, but it was only ever called once, so its
	// calculation is inlined here
	uint16 frequency = note * 4 + (channel.pitchBend - 0x2000) / 171;

	if (frequency >= 508) {
		if (_version >= SCI_VERSION_1_1) {
			frequency = 508;
		} else {
			return;
		}
	}

	const uint8 fNumberIndex = frequency % ARRAYSIZE(frequencyNumbers);
	uint8 fBlockNumber = frequency / ARRAYSIZE(frequencyNumbers);
	if (fBlockNumber) {
		--fBlockNumber;
	}
	assert(fBlockNumber < 8);

	const uint16 frequencyNumber = frequencyNumbers[fNumberIndex];
	assert(frequencyNumber < 0x400);
	sendToHardware(kLowFrequencyNumberRegister + voiceNo, frequencyNumber);

	setVoiceVolume(voiceNo);

	const uint8 value = (noteOn << 5) | (fBlockNumber << 2) | (frequencyNumber >> 8);
	sendToHardware(kHighFrequencyNumberRegister + voiceNo, value);
}

void AdLibDriver::sendOperator(const uint8 opNo, const Operator &op) {
	sendToHardware(kPercussionRegister, 0);
	sendToHardware(kKeySplitRegister, 0);

	const uint8 voiceNo = operatorToVoiceMap[opNo];
	const uint8 registerNo = operatorToRegisterMap[opNo];

	uint8 value = op.keyScaleLevel << 6 | op.outputLevel;
	sendToHardware(kKeyScaleOutputLevelRegister + registerNo, value);

	if ((opNo % 6 / 3) == 0) {
		value = op.feedbackFactor << 1 | !op.isFrequencyModulation;
		sendToHardware(kSynthTypeRegister + voiceNo, value);
	}

	value = op.attackRate << 4 | op.decayRate;
	sendToHardware(kAttackDecayRegister + registerNo, value);

	value = op.sustainLevel << 4 | op.releaseRate;
	sendToHardware(kSustainReleaseRegister + registerNo, value);

	value = op.tremoloOn << 7 | op.vibratoOn << 6 | op.sustainOn << 5 | op.envelopeScalingOn << 4 | op.frequencyMultiplicationFactor;
	sendToHardware(kSavekRegister + registerNo, value);

	sendToHardware(kWaveformSelectRegister + registerNo, op.waveform);
}

void AdLibDriver::sendToHardware(const uint8 registerNo, const uint8 value) {
	if (!_isStereo) {
		_opl->write(0x388, registerNo);
		_opl->write(0x389, value);
	} else {
		sendRight(registerNo, value);
		sendLeft(registerNo, value);
	}
}

void AdLibDriver::sendLeft(const uint8 registerNo, uint8 value) {
	if (registerNo >= kSynthTypeRegister && registerNo <= kSynthTypeRegister + _voices.size() - 1) {
		enum { kEnableLeftSpeaker = 0x10 };
		value |= kEnableLeftSpeaker;
	}

	_opl->write(0x222, registerNo);
	_opl->write(0x223, value);
}

void AdLibDriver::sendRight(const uint8 registerNo, uint8 value) {
	if (registerNo >= kSynthTypeRegister && registerNo <= kSynthTypeRegister + _voices.size() - 1) {
		enum { kEnableRightSpeaker = 0x20 };
		value |= kEnableRightSpeaker;
	}

	_opl->write(0x220, registerNo);
	_opl->write(0x221, value);
}

void AdLibDriver::resetOpl() {
	enum { kMaxRegister = 0xf5 };
	for (uint reg = 0; reg <= kMaxRegister; ++reg) {
		// SSCI just wrote over the entire register range up to kMaxRegister,
		// but since at least the MAME softsynth complains about invalid
		// register writes we skip the registers which are not valid
		if (reg == 0 || reg == 5 || reg == 6 || reg == 7 || (reg >= 9 && reg <= 0x1f)) {
			continue;
		}
		sendToHardware(reg, 0);
	}

	enum { kEnable = 0x20 };
	sendToHardware(kWaveformSelectEnableRegister, kEnable);

	for (uint op = 0; op < kNumOperators; ++op) {
		sendOperator(op, _defaultOperators[op % 6 / 3]);
	}
}

SoundDriver *makeAdLibDriver(ResourceManager &resMan, SciVersion version) {
	return new AdLibDriver(resMan, version);
}

} // End of namespace Sci
