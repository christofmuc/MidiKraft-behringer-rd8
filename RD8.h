#pragma once

#include "Synth.h"
#include "DataFileLoadCapability.h"
#include "StepSequencer.h"
#include "MasterkeyboardCapability.h"
#include "SoundExpanderCapability.h"
#include "GlobalSettingsCapability.h"

#include "MidiController.h"

#include "RD8Pattern.h"

namespace midikraft {

	// Some MIDI constants
	const uint8 RD8_FIRMWARE_MESSAGE = 0x06,
		RD8_DATA_MESSAGE = 0x10,
		RD8_REQUEST = 0x01,
		RD8_STORED_PATTERN_REQUEST = 0x01,
		RD8_STORED_PATTERN_RESPONSE = 0x02,
		RD8_STORED_SONG_REQUEST = 0x03,
		RD8_STORED_SONG_RESPONSE = 0x04,
		RD8_LIVE_PATTERN_REQUEST = 0x05,
		RD8_LIVE_PATTERN_RESPONSE = 0x06,
		RD8_LIVE_SONG_REQUEST = 0x07,
		RD8_LIVE_SONG_RESPONSE = 0x08,
		RD8_GLOBAL_SETTINGS_REQUEST = 0x09,
		RD8_GLOBAL_SETTINGS_RESPONSE = 0x0A,
		RD8_REPLY = 0x02,
		BEHRINGER_ID = 0x32,
		RD8_ID = 0x30;

	template <class T>
	class DataDumpCabability {
	public:
		virtual std::vector<MidiMessage> requestDataDump(T &dump) = 0;
		virtual std::shared_ptr<T> dataFromDumpSysex(const MidiMessage& message, T &unused) const = 0;
		virtual std::vector<MidiMessage> dataToDumpSysex(const T &patch) const = 0;
	};

	class BehringerRD8 : public Synth, public SimpleDiscoverableDevice, public SoundExpanderCapability, public MasterkeyboardCapability, 
		public DataFileLoadCapability, public GlobalSettingsCapability,
		private ValueTree::Listener
	{
	public:
		struct MessageID { uint8 messageType, messageID; };

		enum RD8DateFileTypes {
			STORED_PATTERN,
			STORED_SONG,
			LIVE_PATTERN,
			LIVE_SONG,
			SETTINGS
		};

		BehringerRD8();

		virtual std::shared_ptr<DataFile> patchFromPatchData(const Synth::PatchData &data, MidiProgramNumber place) const override;
		virtual bool isOwnSysex(MidiMessage const &message) const override;

		// Implementation of DiscoverableDevice interface
		virtual std::vector<juce::MidiMessage> deviceDetect(int channel) override;
		virtual int deviceDetectSleepMS() override;
		virtual MidiChannel channelIfValidDeviceResponse(const MidiMessage &message) override;
		virtual bool needsChannelSpecificDetection() override;

		// Implementation of NamedDevice interface
		virtual std::string getName() const override;

		// Helper functions
		std::vector<uint8> createSysexMessage(uint8 deviceID, uint8 messageType, uint8 messageID) const;
		std::vector<uint8> createRequestMessage(MessageID id) const;
		MessageID getMessageID(MidiMessage const &midiMessage) const;

		// DataFileLoadCapability
		virtual std::vector<MidiMessage> requestDataItem(int itemNo, int dataTypeID) override;
		virtual int numberOfDataItemsPerType(int dataTypeID) const override;
		virtual bool isDataFile(const MidiMessage &message, int dataTypeID) const override;
		virtual std::vector<std::shared_ptr<DataFile>> loadData(std::vector<MidiMessage> messages, int dataTypeID) const override;
		std::vector<DataFileDescription> dataTypeNames() const override;

		// Implementation of sequencer interface
		virtual int numberOfSongs() const; // override;
		virtual int numberOfPatternsPerSong() const; // override;
		virtual std::shared_ptr<StepSequencerPattern> activePattern(); // override;
		virtual std::vector<std::shared_ptr<TypedNamedValue>> properties(); // override;

		// SoundExpanderCapability
		virtual bool canChangeInputChannel() const override;

		virtual void changeInputChannel(MidiController *controller, MidiChannel channel, std::function<void()> onFinished) override;
		virtual MidiChannel getInputChannel() const override;
		virtual bool hasMidiControl() const override;
		virtual bool isMidiControlOn() const override;
		virtual void setMidiControl(MidiController *controller, bool isOn) override;

		// MasterkeyboardCapability
		virtual void changeOutputChannel(MidiController *controller, MidiChannel newChannel, std::function<void()> onFinished) override;
		virtual MidiChannel getOutputChannel() const override;
		virtual bool hasLocalControl() const override;
		virtual void setLocalControl(MidiController *controller, bool localControlOn) override;
		virtual bool getLocalControl() const override;
		
		//TODO - this should go away HasBanksCapability
		int numberOfBanks() const override;
		int numberOfPatches() const override;
		std::string friendlyBankName(MidiBankNumber bankNo) const override;

		// GlobalSettingsCapability
		void setGlobalSettingsFromDataFile(std::shared_ptr<DataFile> dataFile) override;
		std::vector<std::shared_ptr<TypedNamedValue>> getGlobalSettings() override;
		DataFileLoadCapability * loader() override;
		int settingsDataFileType() const override;

	private:
		void globalSettingsOperation(MidiController *controller, std::function<void(std::shared_ptr<RD8GlobalSettings> settingsData)> operation);
		void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
		void getMidiChannelsFromDevice();

		struct FirmwareVersion { uint8 major, minor, patch; };

		uint8 deviceID_;
		FirmwareVersion version_;
		std::shared_ptr<RD8Pattern::PatternData> livePattern_; // quasi the edit buffer of the device
		std::vector<std::shared_ptr<TypedNamedValue>> properties_;
		MidiChannel outputChannel_ = MidiChannel::invalidChannel();

		MidiController::HandlerHandle roundtripHandle_ = MidiController::makeNoneHandle();

		std::shared_ptr<RD8GlobalSettings> globalSettings_;
		ValueTree globalSettingsTree_;
	};

}
