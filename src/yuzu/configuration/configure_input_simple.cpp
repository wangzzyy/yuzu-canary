// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <cstring>
#include <functional>
#include <tuple>

#include <QDialog>

#include "ui_configure_input_simple.h"
#include "yuzu/configuration/configure_input.h"
#include "yuzu/configuration/configure_input_player.h"
#include "yuzu/configuration/configure_input_simple.h"
#include "yuzu/ui_settings.h"

template <typename Dialog, typename... Args>
void CallConfigureDialog(ConfigureInputSimple* caller, Args&&... args) {
    caller->applyConfiguration();
    Dialog dialog(caller, std::forward<Args>(args)...);

    const auto res = dialog.exec();
    if (res == QDialog::Accepted) {
        dialog.applyConfiguration();
    }
}

/**
 * OnProfileSelect functions should (when applicable):
 * - Set controller types
 * - Set controller enabled
 * - Set docked mode
 * - Set advanced controller config/enabled (i.e. debug, kbd, mouse, touch)
 *
 * OnProfileSelect function should NOT however:
 * - Reset any button mappings
 * - Open any dialogs
 * - Block in any way
 */

constexpr std::size_t HANDHELD_INDEX = 8;

void HandheldOnProfileSelect(ConfigureInputSimple* caller) {
    Settings::values.players[HANDHELD_INDEX].connected = true;
    Settings::values.players[HANDHELD_INDEX].type = Settings::ControllerType::DualJoycon;

    for (std::size_t player = 0; player < HANDHELD_INDEX; ++player) {
        Settings::values.players[player].connected = false;
    }

    Settings::values.use_docked_mode = false;
    Settings::values.keyboard_enabled = false;
    Settings::values.mouse_enabled = false;
    Settings::values.debug_pad_enabled = false;
    Settings::values.touchscreen.enabled = true;
}

void DualJoyconsDockedOnProfileSelect(ConfigureInputSimple* caller) {
    Settings::values.players[0].connected = true;
    Settings::values.players[0].type = Settings::ControllerType::DualJoycon;

    for (std::size_t player = 1; player <= HANDHELD_INDEX; ++player) {
        Settings::values.players[player].connected = false;
    }

    Settings::values.use_docked_mode = true;
    Settings::values.keyboard_enabled = false;
    Settings::values.mouse_enabled = false;
    Settings::values.debug_pad_enabled = false;
    Settings::values.touchscreen.enabled = false;
}

// Name, OnProfileSelect (called when selected in drop down), OnConfigure (called when configure
// is clicked)
using InputProfile = std::tuple<const char*, std::function<void(ConfigureInputSimple*)>,
                                std::function<void(ConfigureInputSimple*)>>;

const std::array<InputProfile, 3> INPUT_PROFILES{{
    {"Single Player - Handheld - Undocked", HandheldOnProfileSelect,
     [](ConfigureInputSimple* caller) {
         CallConfigureDialog<ConfigureInputPlayer>(caller, HANDHELD_INDEX, false);
     }},
    {"Single Player - Dual Joycons - Docked", DualJoyconsDockedOnProfileSelect,
     [](ConfigureInputSimple* caller) {
         CallConfigureDialog<ConfigureInputPlayer>(caller, 1, false);
     }},
    {"Custom", [](ConfigureInputSimple*) {}, CallConfigureDialog<ConfigureInput>},
}};

ConfigureInputSimple::ConfigureInputSimple(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureInputSimple>()) {
    ui->setupUi(this);

    for (const auto& profile : INPUT_PROFILES) {
        ui->profile_combobox->addItem(std::get<0>(profile), std::get<0>(profile));
    }

    connect(ui->profile_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureInputSimple::OnSelectProfile);
    connect(ui->profile_configure, &QPushButton::pressed, this, &ConfigureInputSimple::OnConfigure);

    this->loadConfiguration();
}

void ConfigureInputSimple::applyConfiguration() {
    auto index = ui->profile_combobox->currentIndex();
    // Make the stored index for "Custom" very large so that if new profiles are added it
    // doesn't change.
    if (std::strncmp(std::get<0>(INPUT_PROFILES.at(index)), "Custom", 6) == 0)
        index = std::numeric_limits<uint32_t>::max();

    UISettings::values.profile_index = index;
}

void ConfigureInputSimple::loadConfiguration() {
    const auto index = UISettings::values.profile_index;
    if (index >= INPUT_PROFILES.size())
        ui->profile_combobox->setCurrentIndex(INPUT_PROFILES.size() - 1);
    else
        ui->profile_combobox->setCurrentIndex(index);
}

void ConfigureInputSimple::OnSelectProfile(int index) {
    std::get<1>(INPUT_PROFILES.at(index))(this);
}

void ConfigureInputSimple::OnConfigure() {
    std::get<2>(INPUT_PROFILES.at(ui->profile_combobox->currentIndex()))(this);
}
