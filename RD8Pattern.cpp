#include "RD8Pattern.h"

#include "MidiHelpers.h"
#include "RD8.h"

#include <boost/format.hpp>

namespace midikraft {

	RD8DataFile::RD8DataFile(BehringerRD8 const *rd8, int dataTypeID, uint8 midiFileType) : DataFile(dataTypeID), rd8_(rd8), midiFileType_(midiFileType)
	{
	}

	bool RD8DataFile::isDataDump(const MidiMessage & message) const
	{
		if (rd8_->isOwnSysex(message)) {
			auto id = rd8_->getMessageID(message);
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

	RD8StoredPattern::RD8StoredPattern(BehringerRD8 const *rd8) : RD8Pattern(rd8, BehringerRD8::STORED_PATTERN, RD8_STORED_PATTERN_RESPONSE)
	{
	}

	std::string RD8StoredPattern::name() const
	{
		return (boost::format("Pattern %02d/%03d") % (int) songNo % (int)patternNo).str();
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
					/*std::vector<uint8> data;
					for (int i = 16; i < message.getSysExDataSize(); i++) {
						data.push_back(message.getSysExData()[i]);
					}*/
					setDataFromSysex(message);
					return true;
				}
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8StoredPattern::dataToSysex() const
	{
		/*auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE, midiFileType_ }));
		message.push_back(songNo);
		message.push_back(patternNo);
		message.insert(message.end(), data().begin(), data().end());*/
		return std::vector<MidiMessage>({ MidiHelpers::sysexMessage(data()) });
	}

	RD8LivePattern::RD8LivePattern(BehringerRD8 const *rd8) : RD8Pattern(rd8, BehringerRD8::LIVE_PATTERN, RD8_LIVE_PATTERN_RESPONSE)
	{
	}

	std::string RD8LivePattern::name() const
	{
		return "Live Pattern";
	}

	bool RD8LivePattern::dataFromSysex(const std::vector<MidiMessage> &messages)
	{
		// At least one of the messages is a data dump, we use the first one to find
		for (auto message : messages) {
			if (isDataDump(message)) {
				// As we don't know the format yet, just copy out all bytes we can get
				/*std::vector<uint8> data;
				for (int i = 14; i < message.getSysExDataSize(); i++) {
					data.push_back(message.getSysExData()[i]);
				}*/
				setDataFromSysex(message);
				return true;
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8LivePattern::dataToSysex() const
	{
		/*auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE,  midiFileType_ }));
		message.insert(message.end(), data().begin(), data().end());*/
		return { MidiHelpers::sysexMessage(data()) };
	}

	RD8StoredSong::RD8StoredSong(BehringerRD8 const *rd8) : RD8Song(rd8, BehringerRD8::STORED_SONG, RD8_STORED_SONG_RESPONSE)
	{
	}

	std::string RD8StoredSong::name() const
	{
		return (boost::format("Stored Song %02d") % (int) songNo ).str();
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
					/*std::vector<uint8> data;
					for (int i = 15; i < message.getSysExDataSize(); i++) {
						data.push_back(message.getSysExData()[i]);
					}*/
					setDataFromSysex(message);
					return true;
				}
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8StoredSong::dataToSysex() const
	{
		/*auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE,  midiFileType_ }));
		message.push_back(songNo);
		message.insert(message.end(), data().begin(), data().end());*/
		return { MidiHelpers::sysexMessage(data()) };
	}

	RD8LiveSong::RD8LiveSong(BehringerRD8 const *rd8) : RD8Song(rd8, BehringerRD8::LIVE_PATTERN, RD8_LIVE_PATTERN_RESPONSE)
	{
	}

	std::string RD8LiveSong::name() const
	{
		return "Live Song";
	}

	bool RD8LiveSong::dataFromSysex(const std::vector<MidiMessage> &messages)
	{
		// At least one of the messages is a data dump, we use the first one to find
		for (auto message : messages) {
			if (isDataDump(message)) {
				// As we don't know the format yet, just copy out all bytes we can get
				/*std::vector<uint8> data; 
				for (int i = 14; i < message.getSysExDataSize(); i++) {
					data.push_back(message.getSysExData()[i]);
				}*/
				setDataFromSysex(message);
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8LiveSong::dataToSysex() const
	{
		/*auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE,  midiFileType_ }));
		message.insert(message.end(), data().begin(), data().end());*/
		return { MidiHelpers::sysexMessage(data()) };
	}

	RD8GlobalSettings::RD8GlobalSettings(BehringerRD8 const *rd8) : RD8DataFile(rd8, BehringerRD8::SETTINGS, RD8_GLOBAL_SETTINGS_RESPONSE)
	{
	}

	std::string RD8GlobalSettings::name() const
	{
		return "Settings";
	}

	bool RD8GlobalSettings::dataFromSysex(const std::vector<MidiMessage> &messages)
	{
		// At least one of the messages is a data dump, we use the first one to find
		for (auto message : messages) {
			if (isDataDump(message)) {
				// As we don't know the format yet, just copy out all bytes we can get
				std::vector<uint8> rawData;
				for (int i = 14; i < message.getSysExDataSize(); i++) {
					rawData.push_back(message.getSysExData()[i]);
				}
				setData(unescapeSysex(rawData));
				globalSettings_.clear();
				// Load the individual data items and create a data structure that will be used by the property panel
				for (auto setting : kGlobalSettingsDefinition) {
					globalSettings_.push_back(std::make_shared<TypedNamedValue>(setting.def)); // Use copy constructor to create shared object
					if (setting.index < data().size()) {
						var dataVariant = at(setting.index);
						globalSettings_.back()->value() = dataVariant;
					}
				}
				return true;
			}
		}
		return false;
	}

	std::vector<juce::MidiMessage> RD8GlobalSettings::dataToSysex() const
	{
		auto message = rd8_->createRequestMessage(BehringerRD8::MessageID({ RD8_DATA_MESSAGE,  midiFileType_ }));
		auto escapedData = escapeSysex(data());
		message.insert(message.end(), escapedData.begin(), escapedData.end());
		return std::vector<MidiMessage>({ MidiHelpers::sysexMessage(message) });
	}

	TypedNamedValueSet RD8GlobalSettings::globalSettings() const
	{
		return globalSettings_;
	}

	bool RD8GlobalSettings::pokeSetting(std::string const &settingName, uint8 newValue)
	{
		for (auto setting : kGlobalSettingsDefinition) {
			if (setting.def.name().toStdString() == settingName) {
				if (newValue >= setting.def.minValue() && newValue <= setting.def.maxValue()) {
					setAt(setting.index, newValue);
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
				return (uint8) at(setting.index);
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
				//{2, "Last Loaded Song"}
				//{3, "Last Loaded Pattern"}
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
				{ 14, TypedNamedValue("Bass Drum MIDI Note", "Note mapping", 0, 0, 128) },
				{ 15, TypedNamedValue("Snare Drum MIDI Note", "Note mapping", 0, 0, 128) },
				{ 16, TypedNamedValue("Low Tom MIDI Note", "Note mapping", 0, 0, 128) },
				{ 17, TypedNamedValue("Mid Tom MIDI Note", "Note mapping", 0, 0, 128) },
				{ 18, TypedNamedValue("High Tom MIDI Note", "Note mapping", 0, 0, 128) },
				{ 19, TypedNamedValue("Rim Shot MIDI Note", "Note mapping", 0, 0, 128) },
				{ 20, TypedNamedValue("Hand Clap MIDI Note", "Note mapping", 0, 0, 128) },
				{ 21, TypedNamedValue("Cow Bell MIDI Note", "Note mapping", 0, 0, 128) },
				{ 22, TypedNamedValue("Cymbal MIDI Note", "Note mapping", 0, 0, 128) },
				{ 23, TypedNamedValue("Open Hat MIDI Note", "Note mapping", 0, 0, 128) },
				{ 24, TypedNamedValue("Closed Hat MIDI Note", "Note mapping", 0, 0, 128) },
				{ 25, TypedNamedValue("Song Chain Mode", "Song mode", false)},
				{ 26, TypedNamedValue("Tempo Preference", "Preferences", 0, kPreferenceLookup) },
				{ 27, TypedNamedValue("Swing Preference", "Preferences", 0, kPreferenceLookup) },
				{ 28, TypedNamedValue("Probability Preference", "Preferences", 0, kPreferenceLookup) },
				{ 29, TypedNamedValue("Flam Preference", "Preferences", 0, kPreferenceLookup) },
				{ 30, TypedNamedValue("Filter Mode Preference", "Preferences", 0, kPreferenceLookup) },
				{ 31, TypedNamedValue("Filter Enable Preference", "Preferences", 0, kPreferenceLookup) },
				{ 32, TypedNamedValue("Filter Automation Preference", "Preferences", 0, kPreferenceLookup) },
				{ 33, TypedNamedValue("Polymeter Preference", "Preferences", 0, kPreferenceLookup) },
				{ 34, TypedNamedValue("Step Size Preference", "Preferences", 0, kPreferenceLookup) },
				{ 35, TypedNamedValue("Auto Advance Preference", "Preferences", 0, {{0, "Song"}, {1, "Global" }}) },
				{ 36, TypedNamedValue("Auto Scroll Preference", "Preferences", 0, { {1, "Global" }, { 2, "Pattern" }}) },
				{ 37, TypedNamedValue("FX Bus Preference", "Preferences", 0, kPreferenceLookup)},
				{ 38, TypedNamedValue("Mute Preference", "Preferences", 0, kPreferenceLookup)},
				{ 39, TypedNamedValue("Solo Preference", "Preferences", 0, kPreferenceLookup)},
				{ 40, TypedNamedValue("Global Tempo", "GlobalSettings", 0, 20, 240) },
				{ 41, TypedNamedValue("Global Swing", "GlobalSettings", 0, 50, 75) },
				{ 42, TypedNamedValue("Global Probability", "GlobalSettings", 0, 0, 100) },
				{ 43, TypedNamedValue("Global Flam", "GlobalSettings", 0, 0, 24) },
				{ 44, TypedNamedValue("Global Filter Mode", "GlobalSettings", false) },
				{ 45, TypedNamedValue("Global Filter Enable", "GlobalSettings", false)},
				{ 46, TypedNamedValue("Global Filter Automation", "GlobalSettings", false)},
				{ 47, TypedNamedValue("Global Filter Steps", "GlobalSettings", 0, 0, 255)}, // This is an interesting editor... needs an array var
				{ 47 + 64 , TypedNamedValue("Global Polymeter", "GlobalSettings", false)},
				{ 47 + 64 + 1 , TypedNamedValue("Global Step Size", "GlobalSettings", false)},
				{ 47 + 64 + 2 , TypedNamedValue("Global Auto-Advance", "GlobalSettings", false)},
				{ 47 + 64 + 3 , TypedNamedValue("Global Auto-Scroll", "GlobalSettings", false)},
				//{ 47 + 64 + 4 , "Global FX Assignments", 0, 1},
				//{ 47 + 64 + 5 , "Global Mute Assignments", 0, 1},
				//{ 47 + 64 + 6 , "Global Solo Assignments", 0, 1},
			};

	std::shared_ptr<RD8Pattern::PatternData> RD8Pattern::getPattern() const
	{
		std::shared_ptr<RD8Pattern::PatternData> result = std::make_shared<RD8Pattern::PatternData>();
		RD8Pattern::PatternData &out = *result;

		// Check that the data version and the product version are ok!
		if (!(at(PatternDataVersion) == 0 && at(ProductVariant) == 0x08) || data().size() != 889) {
			jassert(false);
			return std::shared_ptr<RD8Pattern::PatternData>();
		}

		// Interpret pattern data
		out.tracks.clear();
		for (int track = 0; track < 12; track++) {
			out.tracks.push_back(std::vector<std::shared_ptr<StepData>>());
			for (int step = 0; step < 64; step++) {
				uint8 toDecode = (uint8) at(AccentSteps + track * 64 + step);
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
		out.tempo = (uint8) at(Tempo);
		out.swing = (uint8) at(Swing);
		out.probability = (uint8) at(Probability);
		out.flamLevel = (uint8) at(FlamLevel);
		out.filterMode = (uint8) at(FilterMode);
		jassert(at(FilterEnable) == 0 || at(FilterEnable) == 1); // Assuming this is a bool
		out.filterOnOff = at(FilterEnable);
		jassert(at(FilterAutomation) == 0 || at(FilterAutomation) == 1); // Assuming this is a bool
		out.filterAutomationOnOff = at(FilterAutomation) != 0;
		out.filterSteps.clear();
		for (int i = 0; i < 64; i++) out.filterSteps.push_back((uint8)at(FilterSteps + i));
		jassert(at(PolymeterOnOff) == 0 || at(PolymeterOnOff) == 1); // Assuming this is a bool
		out.polymeterOnOff = at(PolymeterOnOff) != 0;
		out.stepSize = (uint8)at(StepSize);
		jassert(at(AutoAdvance) == 0 || at(AutoAdvance) == 1); // Assuming this is a bool
		out.autoAdvanceOnOff = at(AutoAdvance) != 0;
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
