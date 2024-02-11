import 'dart:convert';

class WiFi {
  final String ssid;
  final String password;
  final String authMode;
  final int rssi;

  WiFi({
    this.password = '',
    this.rssi = 0,
    this.authMode = '',
    this.ssid = '',
  });

  Map<String, dynamic> toMap() {
    return {
      'w_sta_ssid': ssid,
      'w_sta_pass': password,
    };
  }

  factory WiFi.fromMap(Map<String, dynamic> map) {
    return WiFi(
      ssid: map['ssid'] ?? '',
      authMode: map['authmode'] ?? '',
      rssi: map['rssi'] ?? '',
    );
  }

  String toJson() => json.encode(toMap());

  factory WiFi.fromJson(String source) => WiFi.fromMap(json.decode(source));
}

class WiFiMode {
  final String active;

  WiFiMode({
    required this.active,
  });

  Map<String, dynamic> toMap() {
    return {
      'w_sta_active': active,
    };
  }

  String toJson() => json.encode(toMap());
}

class WiFiStatus {
  final String ssid;
  // final String authMode;
  // final int rssi;
  // final String ip4;
  // final String ip6;
  final bool connected;
  final bool active;

  WiFiStatus({
    // required this.authMode,
    // required this.rssi,
    // required this.ip4,
    // required this.ip6,
    required this.connected,
    required this.active,
    this.ssid = '',
  });

  factory WiFiStatus.fromMap(Map<String, dynamic> map) {
    return WiFiStatus(
      // authMode: map['authmode'] ?? "",
      // rssi: map['rssi'] ?? "",
      // ip4: map['ip4'] ?? "",
      // ip6: map['ip6'] ?? "",
      active: map['active'] ?? '',
      connected: map['connected'] ?? '',
      ssid: map['ssid'] ?? '',
    );
  }

  factory WiFiStatus.fromJson(String source) =>
      WiFiStatus.fromMap(json.decode(source));
}
