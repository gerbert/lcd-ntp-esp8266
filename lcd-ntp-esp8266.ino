#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal.h>
#include <IPAddress.h>
#include <time.h>
#include <sys/time.h>
#include <Ticker.h>

#define ARRAY_SZ(x)             (size_t)(sizeof(x) / sizeof(x[0]))

#define LCD_SHIELD_GPIO_D4      4       //! <D4
#define LCD_SHIELD_GPIO_D5      14      //! <D5
#define LCD_SHIELD_GPIO_D6      12      //! <D6
#define LCD_SHIELD_GPIO_D7      13      //! <D7
#define LCD_SHIELD_GPIO_RS      0       //! <D8
#define LCD_SHIELD_GPIO_EN      2       //! <D9
#define LCD_SHIELD_GPIO_BL      15      //! <D10

#define LCD_SHIELD_BRIGHTNESS   128

static time_t seconds = 0;
static time_t minutes = 0;
static time_t hrs = 0;
static time_t days = 0;
static char scroll[256];
static char *ptr = &scroll[0];
static Ticker scroll_line_tck;
static Ticker static_line_tck;
static const char *wifi_sta_ssid = "%some_network%";
static const char *wifi_sta_pass = "%some_secret%";
static bool lock = false;

static LiquidCrystal lcd(LCD_SHIELD_GPIO_RS, LCD_SHIELD_GPIO_EN,
		LCD_SHIELD_GPIO_D4, LCD_SHIELD_GPIO_D5, LCD_SHIELD_GPIO_D6,
		LCD_SHIELD_GPIO_D7);

static char *get_uptime(void) {
	static char uptime[32];

	seconds = millis() / 1000UL;
	minutes = seconds / 60;
	hrs = minutes / 60;
	days = hrs / 24;

	seconds -= (minutes * 60);
	minutes -= (hrs * 60);
	hrs -= (days * 24);

	memset(uptime, 0, ARRAY_SZ(uptime));
	snprintf(uptime, ARRAY_SZ(uptime), "%3dd %02d:%02d:%02d",
			days, hrs, minutes, seconds);

	return uptime;
}

static void scroll_line() {
	char msg[16];

	if (ptr++ == &scroll[strlen(scroll) - 15]) {
		ptr = &scroll[0];
	}

	lcd.setCursor(0, 0);
	memset(msg, 0, ARRAY_SZ(msg));
	snprintf(msg, ARRAY_SZ(msg), "%s", ptr);
	lcd.print(msg);
}

static void static_line() {
	char msg[16];

	memset(msg, 0x20, ARRAY_SZ(msg));
	snprintf(msg, ARRAY_SZ(msg), " %s ", get_uptime());
	lcd.setCursor(0, 1);
	lcd.print(msg);
}

void setup() {
	time_t sec;

	//! <Release the lock
	lock = false;

	//! <Set up brightness pin
	pinMode(LCD_SHIELD_GPIO_BL, OUTPUT);

	//! <Set brightness level
	analogWrite(LCD_SHIELD_GPIO_BL, LCD_SHIELD_BRIGHTNESS);

	//! <Set up LCD display
	lcd.begin(16, 2);

	//! <Set NTP server parameters
	configTime(7200, 3600, "time.nist.gov", "pool.ntp.org");

	//! <Set up WiFi client
	WiFi.mode(WIFI_STA);
	WiFi.begin(wifi_sta_ssid, wifi_sta_pass);

	//! <Wait while WiFi connection is set
	while (WiFi.status() != WL_CONNECTED) {
		lcd.setCursor(0, 0);

		lcd.print("WiFi connecting");
		delay(100);
	}

	lcd.clear();
	lcd.print("WiFi connected");
	delay(500);

	lcd.clear();
	lcd.print("Querying NTP...");
	delay(500);

	while (!sec) {
		time(&sec);
		delay(100);
	}

	lcd.clear();
	lcd.print("Done!");
	delay(500);

	lcd.clear();

	//! <Start dedicated task
	scroll_line_tck.attach(0.500, scroll_line);
	static_line_tck.attach(0.500, static_line);

	lock = true;
}

void loop() {
	struct tm *t_info = NULL;

	timeval tv;
	time_t now;
	IPAddress ip;

	//! <If WiFi/NTP are not set, return
	if (!lock) {
		return;
	}

	//! <Request time data
	gettimeofday(&tv, NULL);
	now = time(&tv.tv_sec);
	t_info = localtime(&now);

	ip = WiFi.localIP();
	memset(scroll, 0x20, ARRAY_SZ(scroll));
	snprintf(scroll, ARRAY_SZ(scroll), "[IP: %s][SSID: %s][RSSI: %4ddB]"
			"[%02d:%02d:%02d %02d.%02d.%04d]                ",
			ip.toString().c_str(), WiFi.SSID().c_str(), WiFi.RSSI(),
			t_info->tm_hour, t_info->tm_min, t_info->tm_sec,
			t_info->tm_mday, (t_info->tm_mon + 1), (t_info->tm_year + 1900));
}
