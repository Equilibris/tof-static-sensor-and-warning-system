# 1 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino"
// = STL
# 3 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2

// = sensor
// == Air quality
# 7 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2
# 8 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2
// == Temprature

// = IO
// == Inter-integrated circuit
# 13 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2
// == Liquid crystal display
# 15 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2

const auto ECO2_TRHES = 1000;
const auto TVOC_TRHES = 300;
const auto TEMP_TRHES_LO = -15.;
const auto TEMP_TRHES_HI = 30.;

const auto BUZZER = 3;
const auto DISABLE = 2;
const auto MENU = A0;
const auto TMP = A1;

enum Menu {
    HOME = 0,
    CO2 = 1,
    TVOC = 2,
    TEMP = 3,
};
struct AirQMeasurement {
public:
    int tvoc; // PP bilion
    int eco2; // PP million
};

Menu menu;
AirQMeasurement measurement;
auto temp = 25.0;

auto alarm_on = true;
auto alarm = false;

auto co2e_ok = true;
auto tvoc_ok = true;
auto temp_ok = true;

rgb_lcd lcd;

void boot_pins () { pinMode(BUZZER, 0x1); pinMode(DISABLE, 0x0); }
void boot_airq () {
    s16 err;
    u16 scaled_ethanol_signal, scaled_h2_signal;
# 63 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino"
    while (sgp_probe() != 0) {
        Serial.println("SGP failed");
        while (1);
    }
    err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal,
                                            &scaled_h2_signal);

    if (err != 0) lcd.println("error reading signals");
    err = sgp_iaq_init();
}
void boot_lcd () { lcd.begin(16, 2); }

void setup() {
    Serial.begin(9600);
    Serial.println("Begin");

    boot_lcd();
    boot_airq();
    boot_pins();
}

void conduct_air_q_scan () {
    const char RETRY_COUNT = 5;

    s16 err = 0;
    for(char i = 0; i < RETRY_COUNT; i++) {
        err = sgp_measure_iaq_blocking_read(
            &measurement.tvoc,
            &measurement.eco2
        );
        if (err == 0) {
            break;
        }
    }
    if (err != 0) Serial.println("ERR");
}
inline void conduct_temp_scan () {
    auto volt = 5.0 * analogRead(TMP)/1024.0;
    temp = (volt - .5) * 100.;
}
inline void conduct_alarm_check () {
    co2e_ok = measurement.eco2 < ECO2_TRHES;
    tvoc_ok = measurement.tvoc < TVOC_TRHES;
    temp_ok = TEMP_TRHES_LO < temp && temp < TEMP_TRHES_HI;

    alarm = alarm_on && !(co2e_ok && tvoc_ok && temp_ok);
}
void run_alarm () {
    if (alarm) tone(BUZZER, 750.0 + 500.0 * sin((float) millis()/200.0));
    else noTone(BUZZER);
}
void query_disable_state() {
    if (digitalRead(DISABLE) == 0x1) {
        alarm_on = !alarm_on;

        while(digitalRead(DISABLE) == 0x1);
    }
}

inline void select_menu () {
    // approx 1023 / 4
    menu = analogRead(MENU) / 300;
}

void pco2 (bool u=true) { lcd.print("CO2:"); lcd.print(measurement.eco2); if(u) lcd.print("ppm"); }
void ptvoc(bool u=true) { lcd.print("TVOC:"); lcd.print(measurement.tvoc); if(u) lcd.print("ppb"); }
void ptemp(bool u=true) { lcd.print("TEMP:"); lcd.print(temp); if(u) lcd.print("c"); }

void next_frame () {
    lcd.clear();
    lcd.setCursor(0,0);
}
void print_alarm () {
    lcd.print( "eCO2:"); lcd.print(co2e_ok ? "OK" : "RK"); // RK = Risk
    lcd.print(" TVOC:"); lcd.print(tvoc_ok ? "OK" : "RK");
    lcd.setCursor(0,1);
    lcd.print( "TEMP:"); lcd.print(temp_ok ? "OK" : "RK");
}
void print_menu () {
    switch (menu) {
        case Menu::HOME:
            pco2(false);
            lcd.print(" ");
            ptvoc(false);
            lcd.setCursor(0,1);
            ptemp(false);
            break;

        case Menu::CO2: pco2(); break;
        case Menu::TVOC: ptvoc(); break;
        case Menu::TEMP: ptemp(); break;
    }
}


void loop() {
    conduct_air_q_scan();

    query_disable_state();

    conduct_alarm_check();
    conduct_temp_scan();
    run_alarm();

    select_menu();

    next_frame();
    if (alarm) print_alarm();
    else print_menu();
}
