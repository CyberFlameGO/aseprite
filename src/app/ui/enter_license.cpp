// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "app/i18n/strings.h"
#include "app/resource_finder.h"
#include "base/fs.h"
#include "enter_license.h"
#include "ui/message.h"
#include "ui/style.h"
#include "ui/system.h"
#include "ver/info.h"

namespace app {

EnterLicense::EnterLicense() : m_timer(500, this), m_activationInProgress(false)
{
  for (auto& layer : message()->style()->layers()) {
    layer.setAlign(layer.align() | ui::WORDWRAP);
  }
  setSizeable(false);
  licenseKey()->Change.connect([this]() {
    licenseKey()->setText(base::string_to_upper(licenseKey()->text()));
    okButton()->setEnabled(licenseKey()->text().size() > 0);
  });

  okButton()->setEnabled(false);
  okButton()->Click.connect([this](ui::Event&) {
    startActivation();
  });

  m_activatedConn = drm::LicenseManager::instance()->Activated.connect(
    [this](drm::ActivatedEvent& ev) { onActivated(ev); });

  m_activationFailedConn = drm::LicenseManager::instance()->ActivationFailed.connect(
    [this](drm::LicenseManager::Exception& e) { onActivationFailed(e); });

  Close.connect([this]() {
    m_activatedConn.disconnect();
    m_activationFailedConn.disconnect();
  });

  m_timer.start();
}

void EnterLicense::onBeforeClose(ui::CloseEvent& ev)
{
  if (m_activationInProgress) {
    ev.cancel();
  }
}

void EnterLicense::onActivationFailed(drm::LicenseManager::Exception& e)
{
  ui::execute_from_ui_thread([this, e]() {
    showError(e.what());
  });
}

void EnterLicense::onActivated(drm::ActivatedEvent& ev)
{
  drm::LicenseManager::instance()->save(ev.getToken());

  ui::execute_from_ui_thread([this, ev]() {
    showSuccess();
  });
}

void EnterLicense::startActivation()
{
  icon()->setVisible(false);
  message()->setText(app::Strings::instance()->enter_license_activating_message());
  layout();
  setEnabled(false);
  std::string key = licenseKey()->text();
  m_activationInProgress = true;
  m_activation = std::thread([this, key]() {
    drm::LicenseManager::instance()->activate(key, get_app_name(), get_app_version());
  });
}


void EnterLicense::showError(const std::string& msg)
{
  if (m_activation.joinable())
    m_activation.join();

  setEnabled(true);
  icon()->setVisible(true);
  message()->setText(msg);
  layout();
  m_activationInProgress = false;
}

void EnterLicense::showSuccess()
{
  if (m_activation.joinable())
    m_activation.join();

  setEnabled(true);
  icon()->setVisible(false);
  message()->setText(app::Strings::instance()->enter_license_activated_message());
  layout();
  okButton()->setEnabled(false);
  m_activationInProgress = false;
}

}