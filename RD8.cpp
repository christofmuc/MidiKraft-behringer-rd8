#include "RD8.h"

#include "MidiHelpers.h"
#include "RD8Pattern.h"
#include "Sysex.h"

namespace midikraft {

	BehringerRD8::BehringerRD8()
	{
		globalSettings_ = std::make_shared<RD8GlobalSettings>(this);
	}

	std::vector<juce::MidiMessage> BehringerRD8::deviceDetect(int channel)
	{
		// The channel is really the device ID, but as it is easy to change with my software, don't rely on the fact that it could be 0!
		return { MidiHelpers::sysexMessage(createSysexMessage((uint8) channel, RD8_FIRMWARE_MESSAGE, RD8_REQUEST)) };
	}

	int BehringerRD8::deviceDetectSleepMS()
	{
		return 120;
	}

	MidiChannel BehringerRD8::channelIfValidDeviceResponse(const MidiMessage &message)
	{
		if (isOwnSysex(message)) {
			auto messageID = getMessageID(message);
			if (messageID.messageType == RD8_FIRMWARE_MESSAGE && messageID.messageID == RD8_REPLY) {
				if (message.getSysExDataSize() > 13) {
					// 7, 8, 9, 10 are reserved according to the manual
					deviceID_ = message.getSysExData()[4];
					version_ = FirmwareVersion({ message.getSysExData()[11], message.getSysExData()[12], message.getSysExData()[13] });
					getMidiChannelsFromDevice();
					return MidiChannel::fromZeroBase(deviceID_); // Again, this is the device ID and not the MIDI channel
				}
			}
		}
		return MidiChannel::invalidChannel();
	}

	bool BehringerRD8::needsChannelSpecificDetection()
	{
		return true;
	}

	std::string BehringerRD8::getName() const
	{
		return "Behringer RD8";
	}

	bool BehringerRD8::isOwnSysex(MidiMessage const &message) const
	{
		if (message.isSysEx() && message.getSysExDataSize() > 3) {
			return message.getSysExData()[0] == 0x00 &&
				message.getSysExData()[1] == 0x20 &&
				message.getSysExData()[2] == BEHRINGER_ID &&
				message.getSysExData()[3] == RD8_ID;
			//message.getSysExData()[4] == deviceID_; Ignored
		}
		return false;
	}

	int BehringerRD8::numberOfBanks() const
	{
		return 1;
	}

	int BehringerRD8::numberOfPatches() const
	{
		return 1;
	}

	std::string BehringerRD8::friendlyBankName(MidiBankNumber bankNo) const
	{
		ignoreUnused(bankNo);
		return "Bank";
	}

	void BehringerRD8::setGlobalSettingsFromDataFile(std::shared_ptr<DataFile> dataFile)
	{
		if (dataFile->dataTypeID() == SETTINGS) {
			auto settings = std::dynamic_pointer_cast<RD8GlobalSettings>(dataFile);
			if (settings) {
				globalSettings_ = settings;
				// Put all of the values into a ValueTree that we will observe
				globalSettingsTree_ = ValueTree("RD8SETTINGS");
				globalSettings_->globalSettings().addToValueTree(globalSettingsTree_);
				globalSettingsTree_.addListener(this);
			}
			else {
				jassertfalse;
			}
		}
	}

	std::vector<std::shared_ptr<TypedNamedValue>> BehringerRD8::getGlobalSettings()
	{
		return globalSettings_->globalSettings();
	}

	midikraft::DataFileLoadCapability * BehringerRD8::loader()
	{
		return this;
	}

	int BehringerRD8::settingsDataFileType() const
	{
		return SETTINGS;
	}

	void BehringerRD8::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
	{
		// Poke the value into the data
		globalSettings_->pokeSetting(String(property.getCharPointer()).toStdString(), (uint8)(int)treeWhosePropertyHasChanged.getProperty(property));

		// Construct MIDI message for this update
		auto update = globalSettings_->dataToSysex();
		if (update.size() == 1) {
			// Then debounced send the new settings to the RD8
			MidiController::instance()->getMidiOutput(midiOutput())->sendMessageDebounced(update[0], 200);
		}
	}

	std::vector<uint8> BehringerRD8::createSysexMessage(uint8 deviceID, uint8 messageType, uint8 messageID) const {
		return std::vector<uint8>({ 0x00, 0x20, BEHRINGER_ID, RD8_ID, deviceID, messageType, messageID });
	}

	std::vector<uint8> BehringerRD8::createRequestMessage(MessageID id) const {
		auto message = createSysexMessage(deviceID_, id.messageType, id.messageID);
		std::vector<uint8> data({ 0x30, 0x00, 0x00, 0x00 /* 4 magic bytes */, version_.major, version_.minor, version_.patch });
		message.insert(message.end(), data.begin(), data.end());
		return message;
	}

	BehringerRD8::MessageID BehringerRD8::getMessageID(MidiMessage const &midiMessage) const {
		if (isOwnSysex(midiMessage)) {
			if (midiMessage.getSysExDataSize() > 6) {
				return MessageID({ midiMessage.getSysExData()[5], midiMessage.getSysExData()[6] });
			}
			else {
				return MessageID({ 0, 0 }); // Invalid message
			}
		}
		return { 0xff, 0xff };
	}

	int BehringerRD8::numberOfSongs() const
	{
		return 16;
	}

	int BehringerRD8::numberOfPatternsPerSong() const
	{
		return 16;
	}

	std::vector<juce::MidiMessage> BehringerRD8::requestDataItem(int itemNo, int dataTypeID)
	{
		std::vector<uint8> message;
		switch (dataTypeID) {
		case STORED_PATTERN:
			message = createRequestMessage(MessageID({ RD8_DATA_MESSAGE, RD8_STORED_PATTERN_REQUEST }));
			message.push_back((uint8)itemNo / 16);
			message.push_back(itemNo % 16);
			break;
		case LIVE_PATTERN:
			message = createRequestMessage(MessageID({ RD8_DATA_MESSAGE, RD8_LIVE_PATTERN_REQUEST }));
			message.push_back((uint8)itemNo / 16);
			break;
		case STORED_SONG:
			message = createRequestMessage(MessageID({ RD8_DATA_MESSAGE, RD8_STORED_SONG_REQUEST }));
			message.push_back((uint8)itemNo);
			break;
		case LIVE_SONG:
			message = createRequestMessage(MessageID({ RD8_DATA_MESSAGE, RD8_LIVE_SONG_REQUEST }));
			break;
		case SETTINGS:
			message = createRequestMessage(MessageID({ RD8_DATA_MESSAGE, RD8_GLOBAL_SETTINGS_REQUEST }));
			break;
		default:
			jassert(false);
		}

		return std::vector<MidiMessage>({ MidiHelpers::sysexMessage(message) });
	}

	int BehringerRD8::numberOfDataItemsPerType(int dataTypeID) const
	{
		switch (dataTypeID) {
		case STORED_PATTERN: return numberOfSongs() * numberOfPatternsPerSong();
		case LIVE_PATTERN: return numberOfPatternsPerSong();
		case STORED_SONG: return numberOfSongs();
		case LIVE_SONG: return 1;
		case SETTINGS: return 1;
		default:
			jassert(false);
		}
		return 0;
	}

	bool BehringerRD8::isDataFile(const MidiMessage &message, int dataTypeID) const
	{
		switch (dataTypeID) {
		case STORED_PATTERN: {
			RD8StoredPattern pattern(this);
			return pattern.isDataDump(message);
		}
		case LIVE_PATTERN: {
			RD8LivePattern pattern(this);
			return pattern.isDataDump(message);
		}
		case STORED_SONG: {
			RD8StoredSong song(this);
			return song.isDataDump(message);
		}
		case LIVE_SONG: {
			RD8LiveSong song(this);
			return song.isDataDump(message);
		}
		case SETTINGS: {
			RD8GlobalSettings settings(this);
			return settings.isDataDump(message);
		}
		default:
			jassert(false);
			return false;
		}
	}

	std::vector<std::shared_ptr<DataFile>> BehringerRD8::loadData(std::vector<MidiMessage> messages, int dataTypeID) const
	{
		std::vector<std::shared_ptr<DataFile>> result;
		for (const auto& message : messages) {
			if (isDataFile(message, dataTypeID)) {
				std::shared_ptr<RD8DataFile> data;
				auto ID = getMessageID(message);
				switch (ID.messageID) {
				case RD8_LIVE_PATTERN_RESPONSE:
					data = std::make_shared<RD8LivePattern>(this);
					if (data->dataFromSysex({ message })) {
						//Sysex::saveSysexIntoNewFile(R"(d:\christof\music\Behringer-RD8\sysex)", "LivePattern", { message });
						result.push_back(data);
					}
					break;
				case RD8_STORED_PATTERN_RESPONSE:
					data = std::make_shared<RD8StoredPattern>(this);
					//Sysex::saveSysexIntoNewFile(R"(d:\christof\music\Behringer-RD8\sysex)", "StoredPattern", { message });
					if (data->dataFromSysex({ message })) {
						result.push_back(data);
					}
					break;
				case RD8_LIVE_SONG_RESPONSE:
					data = std::make_shared<RD8LiveSong>(this);
					//Sysex::saveSysexIntoNewFile(R"(d:\christof\music\Behringer-RD8\sysex)", "LiveSong", { message });
					if (data->dataFromSysex({ message })) {
						result.push_back(data);
					}
					break;
				case RD8_STORED_SONG_RESPONSE:
					data = std::make_shared<RD8StoredSong>(this);
					//Sysex::saveSysexIntoNewFile(R"(d:\christof\music\Behringer-RD8\sysex)", "StoredSong", { message });
					if (data->dataFromSysex({ message })) {
						result.push_back(data);
					}
					break;
				case RD8_GLOBAL_SETTINGS_RESPONSE: {
					auto settingsData = std::make_shared<RD8GlobalSettings>(this);
					//Sysex::saveSysexIntoNewFile(R"(d:\christof\music\Behringer-RD8\sysex)", "GlobalSettingsDump", { message });
					if (settingsData->dataFromSysex({ message })) {
						result.push_back(settingsData);
					}
					break;
				}
				}
			}
		}
		return result;
	}

	std::shared_ptr<StepSequencerPattern> BehringerRD8::activePattern()
	{
		return livePattern_;
	}

	std::vector<std::shared_ptr<TypedNamedValue>> BehringerRD8::properties()
	{
		return properties_;
	}

	bool BehringerRD8::canChangeInputChannel() const
	{
		return true;
	}

	void BehringerRD8::getMidiChannelsFromDevice() {
		globalSettingsOperation(MidiController::instance(), [this](std::shared_ptr<RD8GlobalSettings> settings) {
			uint8 rxChannel = settings->peekSetting("MIDI RX Channel");
			if (rxChannel == 16) {
				setChannel(MidiChannel::omniChannel());
			}
			else if (rxChannel == 17) {
				// Does this happen? Is this the "equal to output channel" value?
				jassert(false);
			}
			else {
				setChannel(MidiChannel::fromZeroBase(rxChannel));
			}
			uint8 txChannel = settings->peekSetting("MIDI TX Channel");
			if (txChannel == 16) {
				outputChannel_ = MidiChannel::omniChannel();
			}
			else {
				outputChannel_ = MidiChannel::fromZeroBase(txChannel);
			}
		});
	}

	void BehringerRD8::globalSettingsOperation(MidiController *controller, std::function<void(std::shared_ptr<RD8GlobalSettings> settingsData)> operation)
	{
		ignoreUnused(controller);
		if (!roundtripHandle_.isNull()) {
			// Another action is still active, don't interrupt it
			jassert(false);
			return;
		}
		MidiController::instance()->enableMidiInput(midiInput());
		MidiController::instance()->addMessageHandler(roundtripHandle_, [this, operation](MidiInput *source, const MidiMessage &message) {
			ignoreUnused(source);
			if (isDataFile(message, SETTINGS)) {
				auto settingsData = std::make_shared<RD8GlobalSettings>(this);
				if (settingsData->dataFromSysex({ message })) {
					operation(settingsData);
				}
				MidiController::instance()->removeMessageHandler(roundtripHandle_);
				roundtripHandle_ = MidiController::makeNoneHandle();
			}
		});
		auto buffer = MidiHelpers::bufferFromMessages(requestDataItem(0, SETTINGS));
		MidiController::instance()->getMidiOutput(midiOutput())->sendBlockOfMessagesNow(buffer);
	}

	void BehringerRD8::changeInputChannel(MidiController *controller, MidiChannel channel, std::function<void()> onFinished)
	{
		globalSettingsOperation(controller, [this, channel, onFinished](std::shared_ptr<RD8GlobalSettings> settings) {
			bool success;
			if (channel.isOmni()) {
				success = settings->pokeSetting("MIDI RX Channel", 16);
			}
			else {
				success = settings->pokeSetting("MIDI RX Channel", (uint8) channel.toZeroBasedInt());
			}

			if (success) {
				// Persist the new input channel in the SimpleDiscoverableDevice base class
				setChannel(channel);
				// Send an update message to the device. Sadly, this is the whole settings page with the new channel patched in
				auto updatedMessage = MidiHelpers::bufferFromMessages(settings->dataToSysex());
				MidiController::instance()->getMidiOutput(midiOutput())->sendBlockOfMessagesNow(updatedMessage);
				onFinished();
			}
		});
	}

	MidiChannel BehringerRD8::getInputChannel() const
	{
		return channel();
	}

	bool BehringerRD8::hasMidiControl() const
	{
		return false;
	}

	bool BehringerRD8::isMidiControlOn() const
	{
		return true;
	}

	void BehringerRD8::setMidiControl(MidiController *controller, bool isOn)
	{
		ignoreUnused(controller, isOn);
		throw std::logic_error("The method or operation is not implemented.");
	}

	void BehringerRD8::changeOutputChannel(MidiController *controller, MidiChannel newChannel, std::function<void()> onFinished)
	{
		globalSettingsOperation(controller, [this, newChannel, onFinished](std::shared_ptr<RD8GlobalSettings> settings) {
			bool success;
			if (newChannel.isOmni()) {
				success = settings->pokeSetting("MIDI TX Channel", 16);
			}
			else {
				success = settings->pokeSetting("MIDI TX Channel", (uint8) newChannel.toZeroBasedInt());
			}

			if (success) {
				// Persist the new input channel in the SimpleDiscoverableDevice base class
				outputChannel_ = newChannel;
				// Send an update message to the device. Sadly, this is the whole settings page with the new channel patched in
				auto updatedMessage = MidiHelpers::bufferFromMessages(settings->dataToSysex());
				MidiController::instance()->getMidiOutput(midiOutput())->sendBlockOfMessagesNow(updatedMessage);
				onFinished();
			}
		});
	}

	MidiChannel BehringerRD8::getOutputChannel() const
	{
		return outputChannel_;
	}

	bool BehringerRD8::hasLocalControl() const
	{
		return false;
	}

	void BehringerRD8::setLocalControl(MidiController *controller, bool localControlOn)
	{
		ignoreUnused(controller, localControlOn);
		throw std::logic_error("The method or operation is not implemented.");
	}

	bool BehringerRD8::getLocalControl() const
	{
		return true;
	}

	std::shared_ptr<midikraft::DataFile> BehringerRD8::patchFromPatchData(const Synth::PatchData &data, MidiProgramNumber place) const
	{
		ignoreUnused(place);
		MidiMessage sysex = MidiHelpers::sysexMessage(data);
		if (isDataFile(sysex, STORED_PATTERN)) {
			auto pattern = std::make_unique<RD8StoredPattern>(this);
			if (pattern->dataFromSysex({ sysex })) {
				return pattern;
			}
		} 
		if (isDataFile(sysex, LIVE_PATTERN)) {
			auto pattern = std::make_unique<RD8LivePattern>(this);
			if (pattern->dataFromSysex({ sysex })) {
				return pattern;
			}
		}
		if (isDataFile(sysex, STORED_SONG)) {
			auto pattern = std::make_unique<RD8StoredSong>(this);
			if (pattern->dataFromSysex({ sysex })) {
				return pattern;
			}
		}
		if (isDataFile(sysex, LIVE_SONG)) {
			auto pattern = std::make_unique<RD8LiveSong>(this);
			if (pattern->dataFromSysex({ sysex })) {
				return pattern;
			}
		}
		return nullptr;
	}

	std::vector<midikraft::DataFileLoadCapability::DataFileDescription> BehringerRD8::dataTypeNames() const
	{
		//TODO - this is a smell, the order has to be equal to the type ID. Dangerous!
		return { { "Stored Pattern", true, true}, { "Stored Song", true, true }, { "Live Pattern", true, true }, { "Live Song", true, true } };
	}

}
