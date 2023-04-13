# 1 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino"
// = STL
# 3 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2

// = sensor
// == Air quality
# 7 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2
# 8 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2
// == Temprature
# 10 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2

// = IO
// == Inter-integrated circuit
# 14 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2
// == Liquid crystal display
# 16 "/home/williams/Dev/MicroProjects/static-sensor-and-warning-system/static-sensor-and-warning-system.ino" 2

const int BUZZER = 3;
const int DISABLE = 2;
const int MENU = A0;

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
float temp_hum_val[2] = {0};
bool alarm = true;

DHT dht(11);
rgb_lcd lcd;

// This can be transformed to non-blocking
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
// Takes .25s
void read_temp () {
    for(int i = 0; i < 1; i++) if (dht.readTempAndHumidity(temp_hum_val)) break;
}

void scheduled_temp_read() {
    static int last = 1;

    const int PAUSE = 5;

    if (last != millis() / (PAUSE * 1000)) {
        last = millis() / (PAUSE * 1000);

        read_temp();
    }
}

void boot_pins () { pinMode(BUZZER, 0x1); pinMode(DISABLE, 0x0); }
void boot_airq () {
    s16 err;
    u16 scaled_ethanol_signal, scaled_h2_signal;







    /*  Init module,Reset all baseline,The initialization takes up to around 15 seconds, during which
        all APIs measuring IAQ(Indoor air quality ) output will not change.Default value is 400(ppm) for co2,0(ppb) for tvoc*/
    while (sgp_probe() != 0) {
        Serial.println("SGP failed");
        while (1);
    }
    /*Read H2 and Ethanol signal in the way of blocking*/
    err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal,
                                            &scaled_h2_signal);

    if (err != 0) lcd.println("error reading signals");
    err = sgp_iaq_init();
}
void boot_lcd () { lcd.begin(16, 2); }
void boot_dht () {
    dht.begin();
}

void run_alarm () {
    if (alarm) tone(BUZZER, 750.0 + 500.0 * sin((float) millis()/200.0));
    else noTone(BUZZER);
}
void query_disable_state() {
    if (digitalRead(DISABLE) == 0x1) {
        alarm = !alarm;

        while(digitalRead(DISABLE) == 0x1);
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println("Begin");

    boot_lcd();
    /* boot_dht(); */
    boot_airq();
    boot_pins();
}

void select_menu () {
    // approx 1023 / 4
    menu = analogRead(MENU) / 300;
}

void pco2 (bool u=true) { lcd.print("CO2 : "); lcd.print(measurement.eco2); if(u) lcd.print("ppm"); }
void ptvoc(bool u=true) { lcd.print("TVOC: "); lcd.print(measurement.tvoc); if(u) lcd.print("ppb"); }
void ptemp(bool u=true) { lcd.print("TEMP: "); lcd.print(temp_hum_val[1]); if(u) lcd.print("c"); }

void print_menu () {
    lcd.clear();
    lcd.setCursor(0, 0);

    switch (menu) {
        case Menu::HOME:
            pco2(false);
            lcd.print(" ");
            ptvoc(false);
            lcd.print(" ");
            ptemp(10);

        case Menu::CO2: pco2(); break;
        case Menu::TVOC: ptvoc(); break;
        case Menu::TEMP: ptemp(); break;
    }
}

void loop() {
    conduct_air_q_scan();
    scheduled_temp_read();

    query_disable_state();
    run_alarm();

    select_menu();
    print_menu();
}