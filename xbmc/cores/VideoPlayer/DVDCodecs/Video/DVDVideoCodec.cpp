/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDVideoCodec.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "windowing/WinSystem.h"
#include <string>
#include <vector>

//******************************************************************************
// VideoPicture
//******************************************************************************

VideoPicture::VideoPicture() = default;

VideoPicture::~VideoPicture()
{
  if (videoBuffer)
  {
    videoBuffer->Release();
  }
}

VideoPicture& VideoPicture::CopyRef(const VideoPicture &pic)
{
  if (videoBuffer)
    videoBuffer->Release();
  *this = pic;
  if (videoBuffer)
    videoBuffer->Acquire();
  return *this;
}

VideoPicture& VideoPicture::SetParams(const VideoPicture &pic)
{
  if (videoBuffer)
    videoBuffer->Release();
  *this = pic;
  videoBuffer = nullptr;
  return *this;
}

VideoPicture::VideoPicture(VideoPicture const&) = default;
VideoPicture& VideoPicture::operator=(VideoPicture const&) = default;

//******************************************************************************
// VideoCodec
//******************************************************************************
bool CDVDVideoCodec::IsSettingVisible(const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data)
{
  if (setting == NULL || value.empty())
    return false;

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVDPAU)
  {
    auto hwaccels = CDVDFactoryCodec::GetHWAccels();
    for (auto &id : hwaccels)
    {
      if (id == "vdpau")
        return true;
    }
    return false;
  }
  else if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVAAPI)
  {
    auto hwaccels = CDVDFactoryCodec::GetHWAccels();
    for (auto &id : hwaccels)
    {
      if (id == "vaapi")
        return true;
    }
    return false;
  }

  // check if we are running on nvidia hardware
  std::string gpuvendor = CServiceBroker::GetRenderSystem().GetRenderVendor();
  std::transform(gpuvendor.begin(), gpuvendor.end(), gpuvendor.begin(), ::tolower);
  bool isNvidia = (gpuvendor.compare(0, 6, "nvidia") == 0);
  bool isIntel = (gpuvendor.compare(0, 5, "intel") == 0);

  // nvidia does only need mpeg-4 setting
  if (isNvidia)
  {
    if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG4)
      return true;

    return false; // will also hide intel settings on nvidia hardware
  }
  else if (isIntel) // intel needs vc1, mpeg-2 and mpeg4 setting
  {
    if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVAAPIMPEG4)
      return true;
    if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVAAPIVC1)
      return true;
    if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVAAPIMPEG2)
      return true;

    return false; // this will also hide nvidia settings on intel hardware
  }
  // if we don't know the hardware we are running on e.g. amd oss vdpau 
  // or fglrx with xvba-driver we show everything
  return true;
}

bool CDVDVideoCodec::IsCodecDisabled(const std::map<AVCodecID, std::string> &map, AVCodecID id)
{
  auto codec = map.find(id);
  if (codec != map.end())
  {
    return (!CServiceBroker::GetSettings().GetBool(codec->second) ||
            !CDVDVideoCodec::IsSettingVisible("unused", "unused",
                                              CServiceBroker::GetSettings().GetSetting(codec->second),
                                              NULL));
  }
  return false; // don't disable what we don't have
}
