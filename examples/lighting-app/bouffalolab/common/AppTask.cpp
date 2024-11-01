/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/identify-server.h>

#include <app/server/Dnssd.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <platform/bouffalolab/common/DiagnosticDataProviderImpl.h>
#include <system/SystemClock.h>

#if HEAP_MONITORING
#include "MemMonitoring.h"
#endif

#if CHIP_ENABLE_OPENTHREAD
#include <platform/OpenThread/OpenThreadUtils.h>
#include <platform/ThreadStackManager.h>
#include <platform/bouffalolab/common/ThreadStackManagerImpl.h>
#include <utils_list.h>
#endif

#if CONFIG_ENABLE_CHIP_SHELL
#include <ChipShellCollection.h>
#include <lib/shell/Engine.h>
#endif

#include <LEDWidget.h>
#include <plat.h>

extern "C" {
#include <bl_gpio.h>
#include <easyflash.h>
#include <hal_gpio.h>
#include <hosal_gpio.h>
#include <hosal_i2c.h>
}

#include "AppTask.h"
#include "mboard.h"
#include <math.h>
#include "demo_pwm.h"
#include <app/server/OnboardingCodesUtil.h>
#include <platform/internal/GenericDeviceInstanceInfoProvider.ipp>

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

#if CONFIG_ENABLE_CHIP_SHELL
using namespace chip::Shell;
#endif

namespace {
static hosal_i2c_dev_t i2c0;

#if defined(BL706_NIGHT_LIGHT) || defined(BL602_NIGHT_LIGHT)
ColorLEDWidget sLightLED;
#else
DimmableLEDWidget sLightLED;
#endif

Identify sIdentify = {
    APP_LIGHT_ENDPOINT_ID,
    AppTask::IdentifyStartHandler,
    AppTask::IdentifyStopHandler,
    Clusters::Identify::IdentifyTypeEnum::kLightOutput,
};

} // namespace

AppTask AppTask::sAppTask;
StackType_t AppTask::appStack[APP_TASK_STACK_SIZE / sizeof(StackType_t)];
StaticTask_t AppTask::appTaskStruct;

#define PIR_GPIO_INT 9
hosal_gpio_dev_t gpio_int = { .port = PIR_GPIO_INT, .config = INPUT_PULL_DOWN, .priv = NULL };

void pir_irq_handler(void * arg)
{
    if (0 == Server::GetInstance().GetFabricTable().FabricCount())
    {
        if(GetAppTask().pirSensorDisable)
        {
            return; // Fabric Network Established
        }
    }
    // ChipLogProgress(NotSpecified, "pir_irq_handler in");
    bl_gpio_intmask((uint8_t)PIR_GPIO_INT, 0);
    GetAppTask().PostEvent(AppTask::APP_EVENT_LIGHTING_PIR);
    // ChipLogProgress(NotSpecified, "3R:pir_irq_handler out");
}

void pir_sensor_init(hosal_gpio_dev_t * pgpio_int)
{
    //ChipLogProgress(NotSpecified, "pir_sensor_init");
    hosal_gpio_init(&gpio_int);
    hosal_gpio_irq_set(&gpio_int, HOSAL_IRQ_TRIG_POS_PULSE, pir_irq_handler, NULL);
    GetAppTask().pirSensorDisable = true;
    ChipLogProgress(NotSpecified, "3R:pir_sensor_init out");
}
void AppTask::get_local_state(local_State_Union_t *nl_state)
{
    size_t saved_value_len = 0;
    ef_get_env_blob(APP_3R_LOCAL_STATE_KEY, &(nl_state->state), sizeof(local_State_Union_t), &saved_value_len);
    ChipLogProgress(NotSpecified, "3R:ef_get_env:nl state 0x%08X", (unsigned int)nl_state->state);
}

void AppTask::set_local_state(local_State_Union_t *nl_state)
{
    ChipLogProgress(NotSpecified, "3R:ef_set_env:nl state 0x%08X", (unsigned int)nl_state->state);
    ef_set_env_blob(APP_3R_LOCAL_STATE_KEY, &(nl_state->state), sizeof(nl_state->state));
}

void AppTask::save_color(uint8_t level, uint8_t hue, uint8_t sat)
{
    ChipLogProgress(NotSpecified, "3R:save_color:level.%d hue.%d sat.%d", level, hue, sat);
    local_State_Union_t *nl_state = &(GetAppTask().local_NL);
    nl_state->byte.lvl = level;
    nl_state->byte.hue = hue;
    nl_state->byte.sat = sat;

    //ChipLogProgress(NotSpecified, "lvl 0x%02X(%d)", nl_state->byte.lvl, nl_state->byte.lvl);
    //ChipLogProgress(NotSpecified, "hue 0x%02X(%d)", nl_state->byte.hue, nl_state->byte.hue);
    //ChipLogProgress(NotSpecified, "sat 0x%02X(%d)", nl_state->byte.sat, nl_state->byte.sat);

    GetAppTask().set_local_state(nl_state);
}

void AppTask::InitLightSensorI2C()
{
    uint8_t temp[4];
    temp[0]=0x00;
    temp[1]=0x02;
    int ret = -1;

    i2c0.port = 0;
    i2c0.config.freq = 100000;                                    /* only support 305Hz~100000Hz */
    i2c0.config.address_width = HOSAL_I2C_ADDRESS_WIDTH_7BIT;     /* only support 7bit */
    i2c0.config.mode = HOSAL_I2C_MODE_MASTER;                     /* only support master */
    i2c0.config.scl = 0;
    i2c0.config.sda = 1;

    ret = hosal_i2c_init(&i2c0);
    if (ret != 0) {
        hosal_i2c_finalize(&i2c0);
        ChipLogProgress(NotSpecified, "3R:InitLightSensorI2C failed!\r\n");
        return;
    }

    ret = hosal_i2c_mem_write(&i2c0, CK_IIC_SLAVE_ADDR, temp[0], HOSAL_I2C_MEM_ADDR_SIZE_8BIT, &temp[1], 1, 500);

    vTaskDelay(200);
    if (ret != 0) {
        hosal_i2c_finalize(&i2c0);
        ChipLogProgress(NotSpecified, "3R:InitLightSensorI2C write failed!\r\n");
        return;
    }
}

void AppTask::InitLightSensor()
{
    uint8_t temp[4];
    temp[0]=0x00;
    temp[1]=0x02;
    int ret = -1;

    i2c0.port = 0;
    i2c0.config.freq = 100000;                                    /* only support 305Hz~100000Hz */
    i2c0.config.address_width = HOSAL_I2C_ADDRESS_WIDTH_7BIT;     /* only support 7bit */
    i2c0.config.mode = HOSAL_I2C_MODE_MASTER;                     /* only support master */
    i2c0.config.scl = 0;
    i2c0.config.sda = 1;

    ret = hosal_i2c_mem_write(&i2c0, CK_IIC_SLAVE_ADDR, temp[0], HOSAL_I2C_MEM_ADDR_SIZE_8BIT, &temp[1], 1, 500);
    vTaskDelay(50);
    if (ret != 0) {
        hosal_i2c_finalize(&i2c0);
        ChipLogProgress(NotSpecified, "3R:InitLightSensor failed!\r\n");
        return;
    }
}

int AppTask::GetLightSensorValue()
{
    uint8_t au8RxBuffer[3] = {0};
    uint8_t regadd[] = {0x0D, 0x0E, 0x0F};
    int ret = -1;

    ret = hosal_i2c_mem_read(&i2c0, CK_IIC_SLAVE_ADDR, regadd[0], HOSAL_I2C_MEM_ADDR_SIZE_8BIT, &au8RxBuffer[0], 1, 300);
    if (ret != 0) {
        hosal_i2c_finalize(&i2c0);
        ChipLogProgress(NotSpecified, "3R:hosal i2c read 0 failed!\r\n");
        return ret;
    }
    ret = hosal_i2c_mem_read(&i2c0, CK_IIC_SLAVE_ADDR, regadd[1], HOSAL_I2C_MEM_ADDR_SIZE_8BIT, &au8RxBuffer[1], 1, 300);
    if (ret != 0) {
        hosal_i2c_finalize(&i2c0);
        ChipLogProgress(NotSpecified, "3R:hosal i2c read 1 failed!\r\n");
        return ret;
    }
    ret = hosal_i2c_mem_read(&i2c0, CK_IIC_SLAVE_ADDR, regadd[2], HOSAL_I2C_MEM_ADDR_SIZE_8BIT, &au8RxBuffer[2], 1, 300);
    if (ret != 0) {
        hosal_i2c_finalize(&i2c0);
        ChipLogProgress(NotSpecified, "3R:hosal i2c read 2 failed!\r\n");
        return ret;
    }

    uint32_t value;
    value = au8RxBuffer[2];
    value = (value<<8) + au8RxBuffer[1];
    value = (value<<8) + au8RxBuffer[0];
    value = value & 0x00FFFFFF;
    value = 2.4 * value;

    uint32_t lightSensorThreshold = 0;
    lightSensorThreshold    =   (value > sAppTask.curLuxValue)
                            ?   (value - sAppTask.curLuxValue)
                            :   (sAppTask.curLuxValue - value);
    if(sAppTask.curLuxValue <= LUX_10) // uint32_t value, always > 0
    {
        if(lightSensorThreshold > LUX_THRESTHOD_0_10) // delta =3
        {
            sAppTask.curLuxValue = value;
        }
    }
    else if(sAppTask.curLuxValue <= LUX_100)
    {
        if(lightSensorThreshold > LUX_THRESTHOD_10_100) // delta = 5
        {
            sAppTask.curLuxValue = value;
        }
    }
    else if(sAppTask.curLuxValue <= LUX_200)
    {
        if(lightSensorThreshold > LUX_THRESTHOD_100_200) // delta = 10
        {
            sAppTask.curLuxValue = value;
        }
    }
    else
    {
        if(lightSensorThreshold >  (sAppTask.curLuxValue / 20)) // delta = *5/100 = 5%
        {
            sAppTask.curLuxValue = value;
        }
    }

    value = (uint16_t)(10000 * log10(value) + 1);

    static uint8_t valueReadCnt = 0;
    if(value == sAppTask.lightSensorValue)
    {
        valueReadCnt++;
        if(valueReadCnt > 10)
        {
            sAppTask.InitLightSensor();
            valueReadCnt = 0;
        }
    }
    sAppTask.lightSensorValue = value;
    return ret;
}

void StartAppTask(void)
{
    GetAppTask().sAppTaskHandle = xTaskCreateStatic(GetAppTask().AppTaskMain, APP_TASK_NAME, ArraySize(GetAppTask().appStack), NULL,
                                                    APP_TASK_PRIORITY, GetAppTask().appStack, &GetAppTask().appTaskStruct);
    if (GetAppTask().sAppTaskHandle == NULL)
    {
        ChipLogError(NotSpecified, "Failed to create app task");
        appError(APP_ERROR_EVENT_QUEUE_FAILED);
    }
}

#if CONFIG_ENABLE_CHIP_SHELL
void AppTask::AppShellTask(void * args)
{
    Engine::Root().RunMainLoop();
}

CHIP_ERROR AppTask::StartAppShellTask()
{
    static TaskHandle_t shellTask;

    Engine::Root().Init();

    cmd_misc_init();

    xTaskCreate(AppTask::AppShellTask, "chip_shell", 1024 / sizeof(configSTACK_DEPTH_TYPE), NULL, APP_TASK_PRIORITY, &shellTask);

    return CHIP_NO_ERROR;
}
#endif

void AppTask::PostEvent(app_event_t event)
{
    if (xPortIsInsideInterrupt())
    {
        BaseType_t higherPrioTaskWoken = pdFALSE;
        xTaskNotifyFromISR(sAppTaskHandle, event, eSetBits, &higherPrioTaskWoken);
    }
    else
    {
        xTaskNotify(sAppTaskHandle, event, eSetBits);
    }
}

void AppTask::AppTaskMain(void * pvParameter)
{
    app_event_t appEvent;
    bool onoff               = false;
    uint64_t currentHeapFree = 0;

#if !(CHIP_DEVICE_LAYER_TARGET_BL702 && CHIP_DEVICE_CONFIG_ENABLE_ETHERNET)
    sLightLED.Init();
#endif
    sAppTask.lightSensorValue = 0;
    sAppTask.curLuxValue = 0;
    sAppTask.InitLightSensorI2C();
    sAppTask.pirSensorDeteched = chip::System::SystemClock().GetMonotonicMilliseconds64().count();
    pir_sensor_init(&gpio_int);

#ifdef BOOT_PIN_RESET
    ButtonInit();

    GetAppTask().get_local_state(&(GetAppTask().local_NL));
    sLightLED.SetColor(GetAppTask().local_NL.byte.lvl, GetAppTask().local_NL.byte.hue, GetAppTask().local_NL.byte.sat);
#else
    /** Without RESET PIN defined, factory reset will be executed if power cycle count(resetCnt) >= APP_REBOOT_RESET_COUNT */
    uint32_t resetCnt      = 0;
    size_t saved_value_len = 0;
    ef_get_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt), &saved_value_len);

    if (resetCnt > APP_REBOOT_RESET_COUNT)
    {
        resetCnt = 0;
        /** To share with RESET PIN logic, mButtonPressedTime is used to recorded resetCnt increased.
         * +1 makes sure mButtonPressedTime is not zero;
         * a power cycle during factory reset confirm time APP_BUTTON_PRESS_LONG will cancel factoryreset */
        GetAppTask().mButtonPressedTime = System::SystemClock().GetMonotonicMilliseconds64().count() + 1;
    }
    else
    {
        resetCnt++;
        GetAppTask().mButtonPressedTime = 0;
    }
    ef_set_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt));
#endif

    GetAppTask().sTimer =
        xTimerCreate("lightTmr", pdMS_TO_TICKS(APP_TIMER_EVENT_DEFAULT_ITVL), false, NULL, AppTask::TimerCallback);
    if (GetAppTask().sTimer == NULL)
    {
        ChipLogError(NotSpecified, "Failed to create timer task");
        appError(APP_ERROR_EVENT_QUEUE_FAILED);
    }

    ChipLogProgress(NotSpecified, "Starting Platform Manager Event Loop");
    CHIP_ERROR ret = PlatformMgr().StartEventLoopTask();
    if (ret != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "PlatformMgr().StartEventLoopTask() failed");
        appError(ret);
    }

    GetAppTask().PostEvent(APP_EVENT_TIMER);
    GetAppTask().PostEvent(APP_EVENT_LIGHTING_MASK);

    vTaskSuspend(NULL);

    DiagnosticDataProviderImpl::GetDefaultInstance().GetCurrentHeapFree(currentHeapFree);
    ChipLogProgress(NotSpecified, "App Task started, with SRAM heap %lld left\r\n", currentHeapFree);

    while (true)
    {
        appEvent                 = APP_EVENT_NONE;
        BaseType_t eventReceived = xTaskNotifyWait(0, APP_EVENT_ALL_MASK, (uint32_t *) &appEvent, portMAX_DELAY);

        if (eventReceived)
        {
            PlatformMgr().LockChipStack();

            if (APP_EVENT_LIGHTING_MASK & appEvent)
            {
                LightingUpdate(appEvent);
            }

            if (APP_EVENT_BTN_SHORT & appEvent)
            {
                ChipLogProgress(NotSpecified, "\r\n\r\n");
                ChipLogProgress(NotSpecified, "3R:-=-=-=-=-=-=- SRAM heap %d left -=-=-=-=-=-=-", xPortGetFreeHeapSize());
                chip::RendezvousInformationFlags rendezvousMode(chip::RendezvousInformationFlag::kBLE);
                PrintOnboardingCodes(rendezvousMode);
                if (Server::GetInstance().GetFabricTable().FabricCount())
                {
                    Clusters::OnOff::Attributes::OnOff::Get(GetAppTask().GetEndpointId(), &onoff);
                    onoff = !onoff;
                    Clusters::OnOff::Attributes::OnOff::Set(GetAppTask().GetEndpointId(), onoff);
                }
                else
                {
                    uint32_t preLuxValue = sAppTask.curLuxValue;
                    GetAppTask().GetLightSensorValue();
                    ChipLogProgress(NotSpecified, "3R: -=-=-=-=-=-=- lightSensor Value: (previous.%d  current.%d) -=-=-=-=-=-=-", (unsigned int)preLuxValue, (unsigned int)sAppTask.curLuxValue);
                    GetAppTask().pirSensorDisable = !GetAppTask().pirSensorDisable;
                    ChipLogProgress(NotSpecified, "3R:-=-=-=-=-=-=- %s pir Sensor -=-=-=-=-=-=-", (!GetAppTask().pirSensorDisable) ? "Enable" : "Disable");
                    sLightLED.Toggle();
                }
            }

#ifdef BOOT_PIN_RESET
            if (APP_EVENT_BTN_LONG & appEvent)
            {
                /** Turn off light to indicate button long press for factory reset is confirmed */
                sLightLED.SetOnoff(false);
            }
#endif
            if (APP_EVENT_IDENTIFY_MASK & appEvent)
            {
                IdentifyHandleOp(appEvent);
            }

            if (APP_EVENT_FACTORY_RESET & appEvent)
            {
                DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
            }

            TimerEventHandler(appEvent);

            PlatformMgr().UnlockChipStack();
        }
    }
}

void AppTask::LightingUpdate(app_event_t status)
{
    uint8_t hue, sat;
    bool onoff;
    DataModel::Nullable<uint8_t> v(0);
    EndpointId endpoint = GetAppTask().GetEndpointId();
    uint16_t temperature;
    Clusters::ColorControl::ColorModeEnum colormode;

    if (APP_EVENT_LIGHTING_MASK & status)
    {

        if (Server::GetInstance().GetFabricTable().FabricCount())
        {
            do
            {
                if (Protocols::InteractionModel::Status::Success != Clusters::OnOff::Attributes::OnOff::Get(endpoint, &onoff))
                {
                    break;
                }

                if (Protocols::InteractionModel::Status::Success !=
                    Clusters::LevelControl::Attributes::CurrentLevel::Get(endpoint, v))
                {
                    break;
                }

                if (Protocols::InteractionModel::Status::Success !=
                    Clusters::ColorControl::Attributes::CurrentHue::Get(endpoint, &hue))
                {
                    break;
                }

                if (Protocols::InteractionModel::Status::Success !=
                    Clusters::ColorControl::Attributes::CurrentSaturation::Get(endpoint, &sat))
                {
                    break;
                }
                if (Protocols::InteractionModel::Status::Success != Clusters::ColorControl::Attributes::ColorTemperatureMireds::Get(endpoint, &temperature)) break;
                if (Protocols::InteractionModel::Status::Success != Clusters::ColorControl::Attributes::ColorMode::Get(endpoint, &colormode)) break;
                ChipLogProgress(NotSpecified,"3R: get_value v.Value=%d hue=%d sat=%d temperature=%d(0x%04x) colormode=%d", v.Value(), hue, sat, temperature, temperature,
                                            (colormode==Clusters::ColorControl::ColorModeEnum::kCurrentHueAndCurrentSaturation)? 0 :
                                            ((colormode==Clusters::ColorControl::ColorModeEnum::kColorTemperatureMireds) ? 2 : 1));

                if (!onoff)
                {
                    sLightLED.SetLevel(0);
                }
                else
                {
                    if (v.IsNull())
                    {
                        v.SetNonNull(254);
                    }
#if defined(BL706_NIGHT_LIGHT) || defined(BL602_NIGHT_LIGHT)
                    ChipLogProgress(NotSpecified,"3R:v.Value=%d hue=%d sat=%d",v.Value(), hue, sat);
                    static bool first_time=true;
                    if(first_time)
                    {
                        // report saved light local state
                        GetAppTask().get_local_state(&(GetAppTask().local_NL));
                        if(0 == GetAppTask().local_NL.state) GetAppTask().local_NL.state = 0x02B78000; // (128, 183, 2)
                        // get saved GetAppTask().local_NL, or default GetAppTask().local_NL (0x02B78000)
                        v.SetNonNull() = GetAppTask().local_NL.byte.lvl;
                        hue = GetAppTask().local_NL.byte.hue;
                        sat = GetAppTask().local_NL.byte.sat;

                        if(GetAppTask().local_NL.byte.resv == 0) { // color_mode:0 HSV
                            // report saved light local state
                            Clusters::LevelControl::Attributes::CurrentLevel::Set(endpoint, v);
                            Clusters::ColorControl::Attributes::CurrentHue::Set(endpoint, hue);
                            Clusters::ColorControl::Attributes::CurrentSaturation::Set(endpoint, sat);
                            Clusters::ColorControl::Attributes::ColorMode::Set(endpoint, Clusters::ColorControl::ColorModeEnum::kCurrentHueAndCurrentSaturation); // color_mode:0 HSV
                        } else if(GetAppTask().local_NL.byte.resv == 2) { // color_mode:2 Temperature
                            temperature = GetAppTask().local_NL.byte.hue;
                            temperature = (temperature << 8) + GetAppTask().local_NL.byte.sat;
                            Clusters::LevelControl::Attributes::CurrentLevel::Set(endpoint, v);
                            Clusters::ColorControl::Attributes::ColorTemperatureMireds::Set(endpoint, temperature);
                            Clusters::ColorControl::Attributes::ColorMode::Set(endpoint, Clusters::ColorControl::ColorModeEnum::kColorTemperatureMireds); // color_mode:2 Temperature
                        } else {
                            ChipLogProgress(NotSpecified,"3R: ColorMode Error");
                        }

                        // report default sensor state
                        Clusters::OccupancySensing::Attributes::Occupancy::Set(3, 0x0001); // pir un-deteched
                        Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Set(2, (uint16_t)sAppTask.lightSensorValue);
                        first_time = false;
                        break;
                    }
                    if(colormode == Clusters::ColorControl::ColorModeEnum::kColorTemperatureMireds) {
                        sLightLED.SetLevelTemperature(v.Value(), temperature);
                        GetAppTask().local_NL.byte.resv = 2; // color_mode:2 Temperature
                        sat = uint8_t( temperature & 0x00FF); // temperature_low
                        hue = uint8_t((temperature>>8) & 0x00FF); // temperature_high
                        GetAppTask().save_color(v.Value(), hue, sat);
                        break;
                    }
                    sLightLED.SetColor(v.Value(), hue, sat);
                    GetAppTask().local_NL.byte.resv = 0; // color_mode:0 HSV
                    GetAppTask().save_color(v.Value(), hue, sat);
#else
                    sLightLED.SetLevel(v.Value());
#endif
                }
            } while (0);
        }
        else
        {
#if defined(BL706_NIGHT_LIGHT) || defined(BL602_NIGHT_LIGHT)
            /** show yellow to indicate not-provision state for extended color light */
            static bool first_time = false;
            if(first_time) return;
            first_time = true;
            ChipLogProgress(NotSpecified,"Yellow*3, and White on");
            sLightLED.SetColor(84, 35, 254);
            vTaskDelay(250);
            sLightLED.SetColor(0, 35, 254);
            vTaskDelay(250);
            sLightLED.SetColor(84, 35, 254);
            vTaskDelay(250);
            sLightLED.SetColor(0, 35, 254);
            vTaskDelay(250);
            sLightLED.SetColor(84, 35, 254);
            vTaskDelay(250);
            sLightLED.SetColor(0, 35, 254);
            vTaskDelay(250);
            sLightLED.SetColor(84, 0, 0);
#else
            /** show 30% brightness to indicate not-provision state */
            sLightLED.SetLevel(25);
#endif
        }
    }
}

bool AppTask::StartTimer(void)
{
    if (xTimerIsTimerActive(GetAppTask().sTimer))
    {
        CancelTimer();
    }

    if (GetAppTask().mTimerIntvl == 0)
    {
        GetAppTask().mTimerIntvl = APP_TIMER_EVENT_DEFAULT_ITVL;
    }

    if (xTimerChangePeriod(GetAppTask().sTimer, pdMS_TO_TICKS(GetAppTask().mTimerIntvl), pdMS_TO_TICKS(100)) != pdPASS)
    {
        ChipLogProgress(NotSpecified, "Failed to access timer with 100 ms delay.");
    }

    return true;
}

void AppTask::CancelTimer(void)
{
    xTimerStop(GetAppTask().sTimer, 0);
}

void AppTask::TimerCallback(TimerHandle_t xTimer)
{
    GetAppTask().PostEvent(APP_EVENT_TIMER);
}

void AppTask::TimerEventHandler(app_event_t event)
{
    uint32_t pressedTime = 0;

    if (GetAppTask().mButtonPressedTime)
    {
        pressedTime = System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime;
#ifdef BOOT_PIN_RESET
        static bool set_red_light=true; // button pressed, SetColor(84, 0, 210). Set once, not always.
        if (ButtonPressed())
        {
            if (pressedTime > APP_BUTTON_PRESS_LONG)
            {
                GetAppTask().PostEvent(APP_EVENT_BTN_LONG);
                if(set_red_light)
                {
                    ChipLogProgress(NotSpecified,"3R:SetColor:(84, 0, 210)");
                    sLightLED.SetColor(84, 0, 210);
                    set_red_light = false;
                }
            }
            else
            {
                set_red_light=true; // set true when button press.
            }

            if (pressedTime >= APP_BUTTON_PRESS_SHORT)
            {
#if defined(BL602_NIGHT_LIGHT) || defined(BL706_NIGHT_LIGHT)
                /** change color to indicate to wait factory reset confirm */
                sLightLED.SetColor(254, 0, 210);
#else
                /** toggle led to indicate to wait factory reset confirm */
                sLightLED.Toggle();
#endif
            }
        }
        else
        {
            if (pressedTime >= APP_BUTTON_PRESS_LONG)
            {
                GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
            }
            else if (APP_BUTTON_PRESS_SHORT >= pressedTime && pressedTime >= APP_BUTTON_PRESS_JITTER)
            {
                GetAppTask().PostEvent(APP_EVENT_BTN_SHORT);
            }
            else
            {
                GetAppTask().PostEvent(APP_EVENT_LIGHTING_MASK);
            }

            GetAppTask().mTimerIntvl        = APP_BUTTON_PRESSED_ITVL;
            GetAppTask().mButtonPressedTime = 0;
        }
#else
        if (pressedTime > APP_BUTTON_PRESS_LONG)
        {
            /** factory reset confirm timeout */
            GetAppTask().mButtonPressedTime = 0;
            GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
        }
        else
        {
#if defined(BL602_NIGHT_LIGHT) || defined(BL706_NIGHT_LIGHT)
            /** change color to indicate to wait factory reset confirm */
            ChipLogProgress(NotSpecified,"3R:SetColor:(84, 0, 210)");
            sLightLED.SetColor(84, 0, 210);
#else
            /** toggle led to indicate to wait factory reset confirm */
            sLightLED.Toggle();
#endif
        }
#endif
    }
#ifdef BOOT_PIN_RESET
    else
    {
        if (ButtonPressed())
        {
            GetAppTask().mTimerIntvl        = APP_BUTTON_PRESSED_ITVL;
            GetAppTask().mButtonPressedTime = System::SystemClock().GetMonotonicMilliseconds64().count();
        }
    }
#endif

    if (Server::GetInstance().GetFabricTable().FabricCount()) // Fabric Network Established
    {
        static uint64_t lightSensorReadTime = 0;
        static bool first_detected_refresh = true;
        // report pir sensor state
        if (APP_EVENT_LIGHTING_PIR & event) // detection.1  set-value, detection.2.3.4... refresh pirSensorDeteched
        {
            ChipLogProgress(NotSpecified, "3R: Detected Yes");
            if(first_detected_refresh)
            {
                Clusters::OccupancySensing::Attributes::Occupancy::Set(3, 0x0001); // pir deteched
                //sAppTask.pirSensorDeteched = chip::System::SystemClock().GetMonotonicMilliseconds64().count();
                first_detected_refresh = false;
            }
            sAppTask.pirSensorDeteched = chip::System::SystemClock().GetMonotonicMilliseconds64().count();
        }

        if(chip::System::SystemClock().GetMonotonicMilliseconds64().count() - sAppTask.pirSensorDeteched > APP_PIR_SENSOR_DETECTED_TIME_MS) // Detect Interval (30s)
        {
            if(!first_detected_refresh){
                Clusters::OccupancySensing::Attributes::Occupancy::Set(3, 0x0000); // pir un-detected
                ChipLogProgress(NotSpecified, "3R: Detected No");
            }
            sAppTask.pirSensorDeteched = chip::System::SystemClock().GetMonotonicMilliseconds64().count();
            first_detected_refresh = true;
        }

        // report MeasuredValue
        if((lightSensorReadTime == 0) || (chip::System::SystemClock().GetMonotonicMilliseconds64().count() - lightSensorReadTime > 1000 )) // Report Interval 1000ms
        {
            uint32_t preLuxValue = sAppTask.curLuxValue;
            GetAppTask().GetLightSensorValue();
            if(preLuxValue !=  sAppTask.curLuxValue) // when differ > 5%,  update curLuxValue, then report lightSensorValue
            {
                Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Set(2, (uint16_t)sAppTask.lightSensorValue);
                ChipLogProgress(NotSpecified, "3R:-=-=-=-=-=-=- lightSensorValue Report 0x%08X -=-=-=-=-=-=-", (unsigned int)sAppTask.lightSensorValue);
                ChipLogProgress(NotSpecified, "3R:-=-=-=-=-=-=- (previous.%d  current.%d) -=-=-=-=-=-=-", (unsigned int)preLuxValue, (unsigned int)sAppTask.curLuxValue);
            }
            lightSensorReadTime = chip::System::SystemClock().GetMonotonicMilliseconds64().count();
        }
    }
    else
    {
        if (APP_EVENT_LIGHTING_PIR & event) // for Massproduction
        {
            ChipLogProgress(NotSpecified, "3R: Detected Yes");
        }
    }

    StartTimer();
}

void AppTask::IdentifyStartHandler(Identify *)
{
    GetAppTask().PostEvent(APP_EVENT_IDENTIFY_START);
}

void AppTask::IdentifyStopHandler(Identify *)
{
    GetAppTask().PostEvent(APP_EVENT_IDENTIFY_STOP);
}

void AppTask::IdentifyHandleOp(app_event_t event)
{
    static uint32_t identifyState = 0;

    if (APP_EVENT_IDENTIFY_START & event)
    {
        identifyState = 1;
        ChipLogProgress(NotSpecified, "identify start");
    }

    if ((APP_EVENT_IDENTIFY_IDENTIFY & event) && identifyState)
    {
        sLightLED.Toggle();
        ChipLogProgress(NotSpecified, "identify");
    }

    if (APP_EVENT_IDENTIFY_STOP & event)
    {
        identifyState = 0;
        GetAppTask().PostEvent(APP_EVENT_LIGHTING_MASK);
        ChipLogProgress(NotSpecified, "identify stop");
    }
}

void AppTask::ButtonEventHandler(uint8_t btnIdx, uint8_t btnAction)
{
    GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
}

#ifdef BOOT_PIN_RESET
static hosal_gpio_dev_t gpio_key = { .port = BOOT_PIN_RESET, .config = INPUT_HIGH_IMPEDANCE, .priv = NULL };

void AppTask::ButtonInit(void)
{
    GetAppTask().mButtonPressedTime = 0;

    hosal_gpio_init(&gpio_key);
    hosal_gpio_irq_set(&gpio_key, HOSAL_IRQ_TRIG_POS_PULSE, GetAppTask().ButtonEventHandler, NULL);
}

bool AppTask::ButtonPressed(void)
{
    uint8_t val = 1;

    hosal_gpio_input_get(&gpio_key, &val);

    return val == 1;
}

void AppTask::ButtonEventHandler(void * arg)
{
    if (ButtonPressed())
    {
        if(GetAppTask().mButtonPressedTime)
        {
            if(System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime > APP_BUTTON_PRESS_JITTER) return; // ALready pressed.
        }
        GetAppTask().PostEvent(APP_EVENT_BTN_ISR);
    }
}
#endif
