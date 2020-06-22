#pragma once

#include "SysexDataSerializationCapability.h"
#include "Patch.h"

#include "StepSequencer.h"

namespace midikraft {

	class BehringerRD8;

	class RD8DataFile : public DataFile, public SysexDataSerializationCapability {
	public:
		RD8DataFile(BehringerRD8 const *rd8, int dataTypeID, uint8 midiFileType);

		virtual std::string name() const override;

		bool isDataDump(const MidiMessage & message) const;

	protected:
		std::vector<uint8> unescapeSysex(const std::vector<uint8> &input) const;
		std::vector<juce::uint8> escapeSysex(const std::vector<uint8> &input) const;

		BehringerRD8 const *rd8_; // The data format serialization depends e.g. on the firmware version of the RD8, so we need a concrete instance!
		uint8 midiFileType_; // The MIDI type identifier for this file
	};

	class RD8Pattern : public RD8DataFile {
	public:
		using RD8DataFile::RD8DataFile;

		class StepData : public StepSequencerStep {
		public:
			virtual bool isOn() override;

			bool stepOnOff;
			bool probabilityOnOff;
			bool flamOnOff;
			bool repeatOnOff;
			uint8 repeat;
		};

		class PatternData : public StepSequencerPattern {
		public:
			virtual int numberOfTracks() const override;
			virtual std::vector<std::string> trackNames() const override;
			virtual std::vector<std::shared_ptr<StepSequencerStep>> track(int trackNo) override;

			// Notes
			std::vector<std::vector<std::shared_ptr<StepData>>> tracks;

			// Settings
			uint8 tempo;
			uint8 swing;
			uint8 probability;
			uint8 flamLevel;
			uint8 filterMode; // 0 == LPF, 1 == HPF
			bool filterOnOff;
			bool filterAutomationOnOff;
			std::vector<uint8> filterSteps;
			bool polymeterOnOff;
			uint8 stepSize;
			bool autoAdvanceOnOff;
		};

		std::shared_ptr<RD8Pattern::PatternData> getPattern() const;

	protected:
		enum SysexIndex {
			// Pattern data
			PatternDataVersion = 0,
			ProductVariant = 1,
			AccentSteps = 2 + 0 * 64,
			BassDrumSteps = 2 + 1 * 64,
			SnareDrumSteps = 2 + 2 * 64,
			LowTomSteps = 2 + 3 * 64,
			MidTomSteps = 2 + 4 * 64,
			HiTomSteps = 2 + 5 * 64,
			RomShotSteps = 2 + 6 * 64,
			HandClapSteps = 2 + 7 * 64,
			CowBellSteps = 2 + 8 * 64,
			CymbalSteps = 2 + 9 * 64,
			OpenHatSteps = 2 + 10 * 64,
			ClosedHatSteps = 2 + 11 * 64,
			PatternLength = 2 + 12 * 64,
			RandomOnOff = 2 + 12 * 64 + 13,
			RandomTracksLo = RandomOnOff + 1, // 8 bit for the first 8 tracks
			RandomTrackHi = RandomTracksLo + 1, // and 4 more bits for the other 4 tracks
			RandomSteps = RandomTrackHi + 1, // 64 steps for random on/off? But there are only 18 bytes left...
			Next = RandomSteps + 18,

			// Pattern parameters
			Tempo = 804, // It is known that this is 804 in version 0 of the data file format
			Swing = Tempo + 1,
			Probability = Swing + 1,
			FlamLevel = Probability + 1,
			FilterMode = FlamLevel + 1,
			FilterEnable = FilterMode + 1,
			FilterAutomation = FilterEnable + 1,
			FilterSteps = FilterAutomation + 1,
			PolymeterOnOff = FilterSteps + 64,
			StepSize = PolymeterOnOff + 1,
			AutoAdvance = StepSize + 1,
			FXBusSends = AutoAdvance + 1,
			//MuteVoices = FXBusSends + 3, unknown I need 11 bits for mute and solo information
			//SoloVoices = MuteVoices + 3
		};

		enum BitPatterns {
			STEP_BYTE_MASK_ON_OFF_BIT = 1 << 0,
			STEP_BYTE_MASK_PROBABILITY_BIT = 1 << 2,
			STEP_BYTE_MASK_FLAM_BIT = 1 << 3,
			STEP_BYTE_MASK_NOTE_REPEAT_ON_OFF_BIT = 1 << 4,
			STEP_BYTE_MASK_NOTE_REPEAT = 3 << 5
		};
	};

	class RD8LivePattern : public RD8Pattern {
	public:
		RD8LivePattern(BehringerRD8 const *rd8);

		virtual bool dataFromSysex(const std::vector<MidiMessage> &message) override;
		virtual std::vector<MidiMessage> dataToSysex() const override;
	};

	class RD8StoredPattern : public RD8Pattern {
	public:
		RD8StoredPattern(BehringerRD8 const *rd8);

		virtual bool dataFromSysex(const std::vector<MidiMessage> &message) override;
		virtual std::vector<MidiMessage> dataToSysex() const override;

	private:
		uint8 songNo;
		uint8 patternNo;

	};

	class RD8Song : public RD8DataFile {
	public:
		using RD8DataFile::RD8DataFile;
	};

	class RD8StoredSong : public RD8Song {
	public:
		RD8StoredSong(BehringerRD8 const *rd8);

		virtual bool dataFromSysex(const std::vector<MidiMessage> &message) override;
		virtual std::vector<MidiMessage> dataToSysex() const override;

	private:
		uint8 songNo;
	};

	class RD8LiveSong : public RD8Song {
	public:
		RD8LiveSong(BehringerRD8 const *rd8);

		virtual bool dataFromSysex(const std::vector<MidiMessage> &message) override;
		virtual std::vector<MidiMessage> dataToSysex() const override;
	};

	class RD8GlobalSettings : public RD8DataFile {
	public:
		RD8GlobalSettings(BehringerRD8 const *rd8);

		virtual bool dataFromSysex(const std::vector<MidiMessage> &message) override;
		virtual std::vector<MidiMessage> dataToSysex() const override;

		// high level access
		std::vector<std::shared_ptr<TypedNamedValue>> globalSettings() const;

		// low level access
		bool pokeSetting(std::string const &settingName, uint8 newValue);
		uint8 peekSetting(std::string const &settingName) const;

	private:
		struct ValueDefinition {
			int index;
			TypedNamedValue def;
		};

		static std::vector<ValueDefinition> kGlobalSettingsDefinition;
	};

}
