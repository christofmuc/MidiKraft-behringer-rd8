#include "RD8Pattern.h"

#include "MidiHelpers.h"
#include "RD8.h"

namespace midikraft {

	RD8DataFile::RD8DataFile(BehringerRD8 const *rd8, uint8 midiFileType) : DataFile(midiFileType), rd8_(rd8), midiFileType_(midiFileType)
	{
	}

	std::string RD8DataFile::name() const
	{
		//TOOD
		return "RD8 data";
	}

	bool RD8DataFile::isDataDump(const MidiMessage & message) const
	{
		if (BehringerRD8::isOwnSysex(message)) {
			auto id = BehringerRD8::getMessageID(message);
			if (id.messageType == RD8_DATA_MESSAGE && id.messageID == midiFileType_) {
				return true;
			}
		}
		return false;
	}

	std::vector<uint8> RD8DataFile::unescapeSysex(const std::vector<uint8> &input) const
	{
		std::vector<uint8> result;
		int dataIndex = 0;
		while (dataIndex < input.size()) {
			uint8 ms_bits = input.data()[dataIndex];
			dataIndex++;
			for (int i = 0; i < 7; i++) {
				if (dataIndex < input.size()) {
					result.push_back(input.data()[dataIndex] | ((ms_bits & (1 << i)) << (7 - i)));
				}
				dataIndex++;
			}
		}
		return result;
	}

	std::vector<juce::uint8> RD8DataFile::escapeSysex(const std::vector<uint8> &input) const
	{
		std::vector<juce::uint8> result;
		int readIndex = 0;
		while (readIndex < input.size()) {
			// Write an empty msb byte and record the index
			result.push_back(0);
			size_t msbIndex = result.size() - 1;

			// Now append the next 7 bytes from the input, and set their msb byte
			uint8 msb = 0;
			for (int i = 0; i < 7; i++) {
				if (readIndex < input.size()) {
					result.push_back(input.at(readIndex) & 0x7f);
					msb |= (input.at(readIndex) & 0x80) >> (7 - i);
				}
				readIndex++;
			}

			// Then poke the msb back into the result stream
			result.at(msbIndex) = msb;
		}
		return result;
	}

	RD8StoredPattern::RD8StoredPattern(BehringerRD8 const *rd8) : RD8Pattern(rd8, RD8_STORED_PATTERN_RESPONSE)
	{
	}

	bool RD8StoredPattern::dataFromSysex(const std::vector<MidiMessage> &messages)
	{
		// At least one of the messages is a data dump, we use the first one to find
		for (auto message : messages) {
			if (isDataDump(message)) {
				// As we don't know the format yet, just copy out all bytes we can get
				if (message.getSysExDataSize() > 15) {
					songNo = message.getSysExData()[14];
					patternNo = message.getSysExData()[15];
					// The rest is binary data we need to decrypt
					for (int i = 16; i < message.getSysExDataSize(); i++) {
						data.push_back(message.getSysExData()[i]);
					}
					data = unescapeSysex(data);
					return true;
				}
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8StoredPattern::dataToSysex() const
	{
		auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE, midiFileType_ }));
		message.push_back(songNo);
		message.push_back(patternNo);
		message.insert(message.end(), data.begin(), data.end());
		return std::vector<MidiMessage>({ MidiHelpers::sysexMessage(message) });
	}

	RD8LivePattern::RD8LivePattern(BehringerRD8 const *rd8) : RD8Pattern(rd8, RD8_LIVE_PATTERN_RESPONSE)
	{
	}

	bool RD8LivePattern::dataFromSysex(const std::vector<MidiMessage> &messages)
	{
		// At least one of the messages is a data dump, we use the first one to find
		for (auto message : messages) {
			if (isDataDump(message)) {
				// As we don't know the format yet, just copy out all bytes we can get
				for (int i = 14; i < message.getSysExDataSize(); i++) {
					data.push_back(message.getSysExData()[i]);
				}
				data = unescapeSysex(data);
				return true;
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8LivePattern::dataToSysex() const
	{
		auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE,  midiFileType_ }));
		message.insert(message.end(), data.begin(), data.end());
		return std::vector<MidiMessage>({ MidiHelpers::sysexMessage(message) });
	}

	RD8StoredSong::RD8StoredSong(BehringerRD8 const *rd8) : RD8Song(rd8, RD8_STORED_SONG_RESPONSE)
	{
	}

	bool RD8StoredSong::dataFromSysex(const std::vector<MidiMessage> &messages)
	{
		// At least one of the messages is a data dump, we use the first one to find
		for (auto message : messages) {
			if (isDataDump(message)) {
				// As we don't know the format yet, just copy out all bytes we can get
				if (message.getSysExDataSize() > 14) {
					songNo = message.getSysExData()[14];
					// The rest is binary data we need to decrypt
					for (int i = 15; i < message.getSysExDataSize(); i++) {
						data.push_back(message.getSysExData()[i]);
					}
					return true;
				}
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8StoredSong::dataToSysex() const
	{
		auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE,  midiFileType_ }));
		message.push_back(songNo);
		message.insert(message.end(), data.begin(), data.end());
		return std::vector<MidiMessage>({ MidiHelpers::sysexMessage(message) });
	}

	RD8LiveSong::RD8LiveSong(BehringerRD8 const *rd8) : RD8Song(rd8, RD8_LIVE_PATTERN_RESPONSE)
	{
	}

	bool RD8LiveSong::dataFromSysex(const std::vector<MidiMessage> &messages)
	{
		// At least one of the messages is a data dump, we use the first one to find
		for (auto message : messages) {
			if (isDataDump(message)) {
				// As we don't know the format yet, just copy out all bytes we can get
				for (int i = 14; i < message.getSysExDataSize(); i++) {
					data.push_back(message.getSysExData()[i]);
				}
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8LiveSong::dataToSysex() const
	{
		auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE,  midiFileType_ }));
		message.insert(message.end(), data.begin(), data.end());
		return std::vector<MidiMessage>({ MidiHelpers::sysexMessage(message) });
	}

	RD8GlobalSettings::RD8GlobalSettings(BehringerRD8 const *rd8) : RD8DataFile(rd8, RD8_GLOBAL_SETTINGS_RESPONSE)
	{
	}

	bool RD8GlobalSettings::dataFromSysex(const std::vector<MidiMessage> &messages)
	{
		// At least one of the messages is a data dump, we use the first one to find
		for (auto message : messages) {
			if (isDataDump(message)) {
				// As we don't know the format yet, just copy out all bytes we can get
				for (int i = 14; i < message.getSysExDataSize(); i++) {
					data.push_back(message.getSysExData()[i]);
				}
				data = unescapeSysex(data);
				return true;
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8GlobalSettings::dataToSysex() const
	{
		auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE,  midiFileType_ }));
		auto escapedData = escapeSysex(data);
		message.insert(message.end(), escapedData.begin(), escapedData.end());
		return std::vector<MidiMessage>({ MidiHelpers::sysexMessage(message) });
	}

	std::vector<std::shared_ptr<TypedNamedValue>> RD8GlobalSettings::globalSettings() const
	{
		std::vector<std::shared_ptr<TypedNamedValue>> settings;

		if (data[0] != 0 || data[1] != 0x08) {
			jassert(false);
			return settings;
		}

		// Load the individual data items and create a data structure that will be used by the property panel
		for (auto setting : kGlobalSettingsDefinition) {
			var dataVariant = data[setting.index];
			settings.push_back(std::make_shared<TypedNamedValue>(setting.def)); // Use copy constructor to create shared object
		}
		return settings;
	}

	bool RD8GlobalSettings::pokeSetting(std::string const &settingName, uint8 newValue)
	{
		for (auto setting : kGlobalSettingsDefinition) {
			if (setting.def.name().toStdString() == settingName) {
				if (newValue >= setting.def.minValue() && newValue <= setting.def.maxValue()) {
					data[setting.index] = newValue;
					return true;
				}
			}
		}
		return false;
	}

	juce::uint8 RD8GlobalSettings::peekSetting(std::string const &settingName) const
	{
		for (auto setting: kGlobalSettingsDefinition) {
			if (setting.def.name().toStdString() == settingName) {
				return data[setting.index];
			}
		}
		jassert(false);
		return 0xff;
	}

	enum SysexIndex {
		// Pattern data
		DataVersion = 0,
		ProductVariant = 1,
		LastLoadedSong = 2,
		LastLoadedPattern = 3,
		DeviceID = 4,
		ClockSource = 5,
		AnalogClockMode = 6
	};

	std::map<int, std::string> kPreferenceLookup = { {0, "Song" }, { 1, "Global" }, { 2, "Pattern" } };
	std::map<int, std::string> kMidiChannelLookup = { {0, "1"}, { 1, "2"}, {2, "3" }, { 3, "4" }, { 4, "5" }, { 5, "6" }, { 6, "7" }, { 7, "8" }, { 8, "9" }, { 9, "10" },
		{10, "11" }, { 11, "12" }, { 12, "13" }, { 13, "14" }, { 14, "15" }, { 15, "16" }, { 16, "All (omni)" } };

	std::vector<RD8GlobalSettings::ValueDefinition> RD8GlobalSettings::kGlobalSettingsDefinition = {
		{ 4, TypedNamedValue("Device ID", "General", 0, 0, 15) },
		{ 5, TypedNamedValue("Clock Source", "General", 0, { {0, "Internal"}, {1, "MIDI" }, { 2, "USB" }, { 3, "Trigger" } }) },
		{ 6, TypedNamedValue("Analog Clock Mode", "General", 0, { {0, "1 PPQ"}, {1, "2 PPQ" }, { 2, "4 PPQ" }, { 3, "24 PPQ" }, { 4, "48 PPQ" } })  },
		{ 7, TypedNamedValue("MIDI RX Channel", "MIDI", 0, kMidiChannelLookup) }, // MIDIChannel with extra!
		{ 8, TypedNamedValue("MIDI TX Channel", "MIDI", 0, kMidiChannelLookup) }, // MIDIChannel with extra!
		{ 9, TypedNamedValue("MIDI to USB through", "MIDI", false) },
		{ 10, TypedNamedValue("MIDI soft through", "MIDI", false) },
		{ 11, TypedNamedValue("USB RX Channel", "MIDI", 0, kMidiChannelLookup) }, // MIDIChannel with extra! 16 = All, 17 = equal to Out
		{ 12, TypedNamedValue("USB TX Channel", "MIDI", 0, kMidiChannelLookup) }, // MIDIChannel with extra!
		{ 13, TypedNamedValue("USB to MIDI through", "MIDI", false) },
		{ 14, TypedNamedValue("Bass Drum MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 15, TypedNamedValue("Snare Drum MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 16, TypedNamedValue("Low Tom MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 17, TypedNamedValue("Mid Tom MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 18, TypedNamedValue("High Tom MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 19, TypedNamedValue("Rim Shot MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 20, TypedNamedValue("Hand Clap MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 21, TypedNamedValue("Cow Bell MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 22, TypedNamedValue("Cymbal MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 23, TypedNamedValue("Open Hat MIDI Note mapping", "Note mapping", 0, 0, 128) },
		{ 24, TypedNamedValue("Closed Hat MIDI Note mapping", "Note mapping", 0, 0, 128) },
	};/*
		{ 25, "Song mode", "Song Chain Mode", ValueType::Bool, 0, 1},
		{ 26, "Preferences", "Tempo Preference", ValueType::Lookup, 0, 2, kPreferenceLookup },
		{ 27, "Preferences", "Swing Preference", ValueType::Lookup,0, 2, kPreferenceLookup },
		{ 28, "Preferences", "Probability Preference", ValueType::Lookup,0, 2, kPreferenceLookup },
		{ 29, "Preferences", "Flam Preference", ValueType::Lookup,0, 2, kPreferenceLookup},
		{ 30, "Preferences", "Filter Mode Preference", ValueType::Lookup,0, 2, kPreferenceLookup},
		{ 31, "Preferences", "Filter Enable Preference", ValueType::Lookup,0, 2, kPreferenceLookup},
		{ 32, "Preferences", "Filter Automation Preference", ValueType::Lookup,0, 2, kPreferenceLookup},
		{ 33, "Preferences", "Polymeter Preference", ValueType::Lookup,0, 2, kPreferenceLookup},
		{ 34, "Preferences", "Step Size Preference", ValueType::Lookup,0, 2, kPreferenceLookup},
		{ 35, "Preferences", "Auto Advance Preference", ValueType::Lookup,0, 1, {{0, "Song"}, {1, "Global" }} },
		{ 36, "Preferences", "Auto Scroll Preference", ValueType::Lookup,1, 2, { {1, "Global" }, { 2, "Pattern" }} },
		{ 37, "Preferences", "FX Bus Preference", ValueType::Lookup,0, 2, kPreferenceLookup},
		{ 38, "Preferences", "Mute Preference", ValueType::Lookup,0, 2, kPreferenceLookup},
		{ 39, "Preferences", "Solo Preference", ValueType::Lookup,0, 2, kPreferenceLookup},
		{ 40, "Global Settings", "Global Tempo", ValueType::Integer, 20, 240},
		{ 41, "Global Settings", "Global Swing", ValueType::Integer, 50, 75},
		{ 42, "Global Settings", "Global Probability", ValueType::Integer, 0, 100},
		{ 43, "Global Settings", "Global Flam", ValueType::Integer, 0, 24},
		{ 44, "Global Settings", "Global Filter Mode", ValueType::Bool, 0, 1},
		{ 45, "Global Settings", "Global Filter Enable", ValueType::Bool, 0, 1},
		{ 46, "Global Settings", "Global Filter Automation", ValueType::Bool, 0, 1},
		{ 47, "Global Settings", "Global Filter Steps", ValueType::Integer, 0, 255}, // This is an interesting editor... needs an array var
		{ 47 + 64 , "Global Settings", "Global Polymeter", ValueType::Bool, 0, 1},
		{ 47 + 64 + 1 , "Global Settings", "Global Step Size", ValueType::Bool, 0, 1},
		{ 47 + 64 + 2 , "Global Settings", "Global Auto-Advance", ValueType::Bool, 0, 1},
		{ 47 + 64 + 3 , "Global Settings", "Global Auto-Scroll", ValueType::Bool, 0, 1},
		//{ 47 + 64 + 4 , "Global FX Assignments", 0, 1},
		//{ 47 + 64 + 5 , "Global Mute Assignments", 0, 1},
		//{ 47 + 64 + 6 , "Global Solo Assignments", 0, 1},
	};*/

	std::shared_ptr<RD8Pattern::PatternData> RD8Pattern::getPattern() const
	{
		std::shared_ptr<RD8Pattern::PatternData> result = std::make_shared<RD8Pattern::PatternData>();
		RD8Pattern::PatternData &out = *result;

		// Check that the data version and the product version are ok!
		if (!(data[PatternDataVersion] == 0 && data[ProductVariant] == 0x08) || data.size() != 889) {
			jassert(false);
			return std::shared_ptr<RD8Pattern::PatternData>();
		}

		// Interpret pattern data
		out.tracks.clear();
		for (int track = 0; track < 12; track++) {
			out.tracks.push_back(std::vector<std::shared_ptr<StepData>>());
			for (int step = 0; step < 64; step++) {
				uint8 toDecode = data[AccentSteps + track * 64 + step];
				std::shared_ptr<RD8Pattern::StepData> stepData = std::make_shared<RD8Pattern::StepData>();
				stepData->stepOnOff = (toDecode & STEP_BYTE_MASK_ON_OFF_BIT) != 0;
				stepData->probabilityOnOff = (toDecode & STEP_BYTE_MASK_PROBABILITY_BIT) != 0;
				stepData->flamOnOff = (toDecode & STEP_BYTE_MASK_FLAM_BIT) != 0;
				stepData->repeatOnOff = (toDecode & STEP_BYTE_MASK_NOTE_REPEAT_ON_OFF_BIT) != 0;
				stepData->repeat = (toDecode & STEP_BYTE_MASK_NOTE_REPEAT) >> 5;
				out.tracks[track].push_back(stepData);
			}
		}

		// Interpret pattern parameters
		out.tempo = data[Tempo];
		out.swing = data[Swing];
		out.probability = data[Probability];
		out.flamLevel = data[FlamLevel];
		out.filterMode = data[FilterMode];
		jassert(data[FilterEnable] == 0 || data[FilterEnable] == 1); // Assuming this is a bool
		out.filterOnOff = data[FilterEnable];
		jassert(data[FilterAutomation] == 0 || data[FilterAutomation] == 1); // Assuming this is a bool
		out.filterAutomationOnOff = data[FilterAutomation] != 0;
		out.filterSteps.clear();
		for (int i = 0; i < 64; i++) out.filterSteps.push_back(data[FilterSteps + i]);
		jassert(data[PolymeterOnOff] == 0 || data[PolymeterOnOff] == 1); // Assuming this is a bool
		out.polymeterOnOff = data[PolymeterOnOff] != 0;
		out.stepSize = data[StepSize];
		jassert(data[AutoAdvance] == 0 || data[AutoAdvance] == 1); // Assuming this is a bool
		out.autoAdvanceOnOff = data[AutoAdvance] != 0;
		return result;
	}

	bool RD8Pattern::StepData::isOn()
	{
		return stepOnOff;
	}

	int RD8Pattern::PatternData::numberOfTracks() const
	{
		return 12;
	}

	std::vector<std::string> RD8Pattern::PatternData::trackNames() const
	{
		return std::vector<std::string>{
			"Accent", "Bass Drum", "Snare Drum", "Low Tom/Conga", "Mid Tom/Conga", "High Tom/Conga", "Rim Shot/Claves", "Hand Clap/Maracas", "Cow Bell", "Cymbal", "Open Hat", "Closed Hat"
		};
	}

	std::vector<std::shared_ptr<StepSequencerStep>> RD8Pattern::PatternData::track(int trackNo)
	{
		std::vector<std::shared_ptr<StepSequencerStep>> result;
		result.insert(result.end(), tracks[trackNo].begin(), tracks[trackNo].end());
		return result;
	}

}
