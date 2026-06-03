#include "../drivers/vga.h"

// Defined in your future net driver
extern void e1000_send(const char* data, unsigned int len);

static int net_enabled = 0;
static int wifi_radio_enabled = 1;
static char connected_ssid[32] = "";
static char local_ip[16] = "10.0.2.15";
static char gateway_ip[16] = "10.0.2.2";

struct wifi_network {
    const char* ssid;
    int channel;
    int signal;
    int secure;
};

static struct wifi_network networks[] = {
    { "ExNet", 6, 92, 1 },
    { "QEMU-Lab", 11, 78, 1 },
    { "Guest", 1, 54, 0 }
};

static int str_eq(char* a, char* b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] == b[i]) {
        if (a[i] == '\0') return 1;
        i++;
    }
    return 0;
}

static int str_len(const char* s) {
    int len = 0;
    while (s && s[len] != '\0') len++;
    return len;
}

static char* skip_spaces(char* s) {
    while (s && *s == ' ') s++;
    return s;
}

static char* split_arg(char* s) {
    if (!s) return 0;

    while (*s != '\0') {
        if (*s == ' ') {
            *s = '\0';
            return skip_spaces(s + 1);
        }
        s++;
    }

    return 0;
}

static void copy_ssid(char* ssid) {
    int i = 0;
    while (ssid[i] != '\0' && i < 31) {
        connected_ssid[i] = ssid[i];
        i++;
    }
    connected_ssid[i] = '\0';
}

static void print_int(int value) {
    char out[12];
    int i = 0;

    if (value == 0) {
        zeal_putc('0');
        return;
    }

    if (value < 0) {
        zeal_putc('-');
        value = -value;
    }

    while (value > 0 && i < 11) {
        out[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i > 0) zeal_putc(out[--i]);
}

static int known_network(char* ssid) {
    int count = sizeof(networks) / sizeof(networks[0]);

    for (int i = 0; i < count; i++) {
        if (str_eq(ssid, (char*)networks[i].ssid)) return 1;
    }

    return 0;
}

static void print_wifi_status() {
    zeal_write("wifictl: radio=");
    zeal_write(wifi_radio_enabled ? "on" : "off");
    zeal_write(" device=simwifi0 backend=e1000\n");

    if (net_enabled) {
        zeal_write("wifictl: state=connected ssid=");
        zeal_write(connected_ssid);
        zeal_write(" ip=");
        zeal_write(local_ip);
        zeal_write(" gateway=");
        zeal_write(gateway_ip);
        zeal_putc('\n');
    } else {
        zeal_write("wifictl: state=disconnected\n");
    }
}

static void scan_wifi() {
    int count = sizeof(networks) / sizeof(networks[0]);

    if (!wifi_radio_enabled) {
        zeal_write("wifictl: radio is off\n");
        return;
    }

    zeal_write("SSID                 CH  SIGNAL  SECURITY\n");
    for (int i = 0; i < count; i++) {
        zeal_write(networks[i].ssid);
        zeal_write("  ");
        print_int(networks[i].channel);
        zeal_write("   ");
        print_int(networks[i].signal);
        zeal_write("%     ");
        zeal_write(networks[i].secure ? "wpa2" : "open");
        zeal_putc('\n');
    }
}

void exaget(char* ip, char* path) {
    char request[256];
    int pos = 0;
    const char* prefix = "GET ";
    const char* http = " HTTP/1.1\r\nHost: ";
    const char* suffix = "\r\nConnection: close\r\n\r\n";

    if (!path || path[0] == '\0') {
        zeal_write("Usage: exaget <path>\n");
        return;
    }

    if (!net_enabled) {
        zeal_write("exaget: network is down. Run wifictl connect ExNet\n");
        return;
    }

    zeal_write("exaget: preparing request...\n");

    for (int i = 0; prefix[i] != '\0' && pos < 255; i++) request[pos++] = prefix[i];
    if (path[0] != '/' && pos < 255) request[pos++] = '/';
    for (int i = 0; path[i] != '\0' && pos < 255; i++) request[pos++] = path[i];
    for (int i = 0; http[i] != '\0' && pos < 255; i++) request[pos++] = http[i];
    for (int i = 0; ip[i] != '\0' && pos < 255; i++) request[pos++] = ip[i];
    for (int i = 0; suffix[i] != '\0' && pos < 255; i++) request[pos++] = suffix[i];
    request[pos] = '\0';

    zeal_write("exaget: sending HTTP request to ");
    zeal_write(ip);
    zeal_write("...\n");

    e1000_send(request, str_len(request));

    zeal_write("exaget: request handed to NIC driver.\n");
    zeal_write("exaget: TCP receive path is not implemented yet.\n");
}

void wifictl(char* args) {
    char* cmd = skip_spaces(args);

    if (!cmd || cmd[0] == '\0' || str_eq(cmd, "status")) {
        print_wifi_status();
        return;
    }

    if (str_eq(cmd, "scan")) {
        scan_wifi();
        return;
    }

    if (str_eq(cmd, "disconnect")) {
        net_enabled = 0;
        connected_ssid[0] = '\0';
        zeal_write("wifictl: disconnected\n");
        return;
    }

    if (str_eq(cmd, "on")) {
        wifi_radio_enabled = 1;
        zeal_write("wifictl: radio on\n");
        return;
    }

    if (str_eq(cmd, "off")) {
        wifi_radio_enabled = 0;
        net_enabled = 0;
        connected_ssid[0] = '\0';
        zeal_write("wifictl: radio off\n");
        return;
    }

    char* value = split_arg(cmd);
    if (str_eq(cmd, "connect")) {
        if (!value || value[0] == '\0') {
            zeal_write("Usage: wifictl connect <ssid>\n");
            return;
        }

        if (!wifi_radio_enabled) {
            zeal_write("wifictl: radio is off\n");
            return;
        }

        if (!known_network(value)) {
            zeal_write("wifictl: warning: unknown SSID, creating profile\n");
        }

        net_enabled = 1;
        copy_ssid(value);
        zeal_write("wifictl: connected to ");
        zeal_write(connected_ssid);
        zeal_putc('\n');
        zeal_write("wifictl: ip=");
        zeal_write(local_ip);
        zeal_write(" gateway=");
        zeal_write(gateway_ip);
        zeal_write(" backend=e1000\n");
        return;
    }

    zeal_write("Usage: wifictl [status|scan|connect <ssid>|disconnect|on|off]\n");
}
