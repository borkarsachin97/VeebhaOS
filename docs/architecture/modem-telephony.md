# Telephony & Baseband Modem Architecture

This document describes the telephony subsystem architecture in VeebhaOS, the rationale for the current mock modem implementation, and the hardware abstraction layer (HAL) designed for real cellular baseband integration.

---

## 1. Subsystem Architecture

VeebhaOS decouples high-level telephony applications (Dialer, SMS, Contacts, In-Call UI, Call Logs) from the physical modem engine through an event-driven Telephony Abstraction Layer:

```text
+--------------------------------------------------------------------------+
|                     Telephony Applications & Shell                       |
|   app_dialer  |  app_messages  |  app_contacts  |  app_incall  |  Status |
+--------------------------------------------------------------------------+
                                     │
                                     ▼
+--------------------------------------------------------------------------+
|                     Telephony Service Manager & Store                    |
|   Call State Machine | Message Inbox/Outbox | Contact Book | Live Pill   |
+--------------------------------------------------------------------------+
                                     │
                                     ▼
+--------------------------------------------------------------------------+
|                         Baseband Modem HAL Contract                      |
|      (hal_modem_init, hal_modem_dial, hal_modem_send_sms, etc.)          |
+----------------------------------+---------------------------------------+
                                   │
         ┌─────────────────────────┴─────────────────────────┐
         ▼                                                   ▼
+----------------------------------+   +-----------------------------------+
|      Mock Telephony Driver       |   |       Physical Baseband Modem     |
|   (Development & Testing)        |   |   (Target Hardware Integration)   |
| * Deterministic unit testing     |   | * AT Command Parser (3GPP 27.007) |
| * Offline simulator execution    |   | * SIM Card APDU Interface (7816)  |
| * Zero cellular infrastructure   |   | * Real-time baseband audio & PCM  |
+----------------------------------+   +-----------------------------------+
```

---

## 2. Rationale for Mock Telephony

During current development phases, VeebhaOS defaults to a deterministic software mock modem for three critical architectural and physical reasons:

### A. 2G Spectrum Sunset
The target development SoC (**RDA8809**) integrates a **2G-only Quad-Band GSM/GPRS baseband**. In many modern regions (North America, East Asia, parts of Europe, and Oceania), commercial 2G networks have already been sunset or severely restricted. Requiring a live 2G cellular network for standard OS development would prevent open-source developers from building and testing applications without specialized, expensive Software-Defined Radio (SDR) equipment (e.g. OsmocomBB / OpenBSC).

### B. Baseband Real-Time Constraints & Timing Accuracy
Real GSM basebands operate with microsecond-precision real-time requirements:
- TDMA frame synchronization (4.615 ms frame intervals).
- Paging sub-channels and burst scheduling.
- Strict Layer 1/2/3 radio protocol stack execution.
Running the full baseband stack concurrently with an unverified user-space application could trigger radio timing violations or baseband watchdog assertions during debugging.

### C. SIM Card State Requirements
A physical baseband modem requires:
- Active electrical negotiation over the ISO 7816 Smart Card interface.
- SIM PIN authentication, IMSI retrieval, and home operator registration.
- Continuous network registration (MCC/MNC search, Location Updating, RSSI measurement).
The Mock Telephony layer provides instantly valid carrier state (`Veebha Mobile`), preloaded contacts, and simulated incoming/outgoing calls without needing physical SIM cards or active subscriptions.

---

## 3. Baseband HAL Contract (`hal_modem.h`)

To connect a physical modem (either the internal RDA8809 baseband or an external 4G/LTE modem module like Quectel or SIMCom via UART), the driver must satisfy the following clean interface:

### Core Modem Lifecycle
```c
typedef enum {
    MODEM_NET_NO_SIGNAL = 0,
    MODEM_NET_SEARCHING,
    MODEM_NET_REGISTERED_HOME,
    MODEM_NET_REGISTERED_ROAMING,
    MODEM_NET_DENIED
} modem_net_state_t;

typedef struct {
    bool (*init)(void);
    bool (*deinit)(void);
    bool (*power_set)(bool on);
    modem_net_state_t (*get_network_state)(void);
    uint8_t (*get_signal_rssi)(void); /* 0..5 bars */
    const char * (*get_operator_name)(void);
} hal_modem_driver_t;
```

### Voice Call Interface
```c
typedef enum {
    CALL_STATE_IDLE = 0,
    CALL_STATE_DIALING,
    CALL_STATE_RINGING,
    CALL_STATE_ACTIVE,
    CALL_STATE_HOLD
} modem_call_state_t;

typedef struct {
    bool (*dial)(const char *number);
    bool (*answer)(void);
    bool (*hangup)(void);
    bool (*send_dtmf)(char tone);
    void (*set_call_status_cb)(void (*cb)(modem_call_state_t state, const char *caller));
} hal_modem_voice_t;
```

### SMS Messaging Interface
```c
typedef struct {
    bool (*send_sms)(const char *recipient, const char *body);
    void (*set_sms_incoming_cb)(void (*cb)(const char *sender, const char *timestamp, const char *body));
} hal_modem_sms_t;
```

---

## 4. Migration Path to Real 4G/VoLTE Hardware

When transitioning from the 2G RDA8809 to modern 4G/VoLTE feature phone chipsets (such as Unisoc T107/T117 or external AT modems):

1. **AT Command Engine**: Implement a standard 3GPP AT-command queue over hardware UART (`ATD`, `ATH`, `ATA`, `+CMGS`, `+CSQ`, `+CREG`).
2. **Audio Routing**: Connect modem PCM I2S lines directly to the audio codec or DAC during active call states.
3. **RI (Ring Indicator) Interrupt**: Wire the modem's hardware RI pin to a GPIO interrupt to wake the OS and display the incoming call screen instantly.
