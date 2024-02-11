// ignore_for_file: use_build_context_synchronously

import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

import 'error_handling.dart';
import '../utils/utils.dart';
import '../models/wifi_model.dart';

class ApiServices {
  static const String ip =
      "http://192.168.2.1"; //IP ON WHICH YOUR ESP SERVER IS RUNNING (CAN BE CHANGED THROUGH ESP SERVER CODE)

  Future<List<WiFi>> scanWifi({
    required BuildContext context,
  }) async {
    List<WiFi> wifiList = [];
    try {
      http.Response res =
          await http.get(Uri.parse('$ip/esp_server/wifi_scan'), headers: {
        'Content-Type': 'application/json; charset=UTF-8',
      });

      httpErrorHandle(
        response: res,
        context: context,
        onSuccess: () {
          for (int i = 0; i < jsonDecode(res.body).length; i++) {
            wifiList.add(
              WiFi.fromJson(
                jsonEncode(
                  jsonDecode(res.body)[i],
                ),
              ),
            );
          }
        },
      );
    } catch (e) {
      showSnackBar(context, e.toString());
    }
    return wifiList;
  }

  void changeWifi({
    required BuildContext context,
    required String ssid,
    required String password,
  }) async {
    try {
      WiFi cred = WiFi(
        ssid: ssid,
        password: password,
      );
      http.Response res = await http.post(
        Uri.parse('$ip/esp_server/wifi_config'),
        body: cred.toJson(),
        headers: <String, String>{
          'Content-Type': 'application/json; charset=UTF-8',
        },
      );

      httpErrorHandle(
        response: res,
        context: context,
        onSuccess: () {
          showSnackBar(
            context,
            'Connected',
          );
        },
      );
    } catch (e) {
      showSnackBar(context, e.toString());
    }
  }

  void changeMode({
    required BuildContext context,
    required String active,
  }) async {
    try {
      WiFiMode mode = WiFiMode(active: active);
      http.Response res = await http.post(
        Uri.parse('$ip/esp_server/wifi_config'),
        body: mode.toJson(),
        headers: <String, String>{
          'Content-Type': 'application/json; charset=UTF-8',
        },
      );
      httpErrorHandle(
        response: res,
        context: context,
        onSuccess: () {
          showSnackBar(
              context, active == "1" ? 'Turned On WiFi' : "Turned Off WiFi");
        },
      );
    } catch (e) {
      showSnackBar(context, e.toString());
    }
  }

  Future<List<WiFiStatus>> getWifiStatus({
    required BuildContext context,
  }) async {
    List<WiFiStatus> status = [];
    try {
      http.Response res =
          await http.get(Uri.parse('$ip/esp_server/device_status'), headers: {
        'Content-Type': 'application/json; charset=UTF-8',
      });

      httpErrorHandle(
        response: res,
        context: context,
        onSuccess: () {
          for (int i = 0; i < jsonDecode(res.body).length; i++) {
            status.add(
              WiFiStatus.fromJson(
                jsonEncode(
                  jsonDecode(res.body)[i],
                ),
              ),
            );
          }
        },
      );
    } catch (e) {
      showSnackBar(context, e.toString());
    }
    return status;
  }
}
