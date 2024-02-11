import 'package:flutter/material.dart';

import '../services/services.dart';

class WifiWidget extends StatefulWidget {
  final String ssid;
  final String authMode;
  final int rssi;
  const WifiWidget(
      {super.key,
      required this.ssid,
      required this.authMode,
      required this.rssi});

  @override
  State<WifiWidget> createState() => _WifiWidgetState();
}

class _WifiWidgetState extends State<WifiWidget> {
  final TextEditingController _passwordController = TextEditingController();

  final ApiServices apiServices = ApiServices();

  void changeWifi() {
    apiServices.changeWifi(
      context: context,
      ssid: widget.ssid,
      password: _passwordController.text,
    );
    Navigator.pop(context);
    apiServices.getWifiStatus(context: context);
    _passwordController.clear();
    setState(() {});
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () {
        showModalBottomSheet(
          backgroundColor: Colors.pink,
          isScrollControlled: true,
          context: context,
          builder: (context) => Padding(
            padding: EdgeInsets.only(
                bottom: MediaQuery.of(context).viewInsets.bottom),
            child: SizedBox(
              height: 230,
              child: Padding(
                padding: const EdgeInsets.all(18.0),
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Container(
                      height: 55,
                      width: double.infinity,
                      decoration: BoxDecoration(
                          color: const Color.fromARGB(57, 13, 58, 131),
                          borderRadius: BorderRadius.circular(18)),
                      child: Padding(
                        padding: const EdgeInsets.all(8.0),
                        child: Text(
                          widget.ssid,
                          style: const TextStyle(
                              fontSize: 30, color: Colors.white),
                        ),
                      ),
                    ),
                    const SizedBox(
                      height: 5,
                    ),
                    Container(
                      height: 55,
                      width: double.infinity,
                      decoration: BoxDecoration(
                          color: const Color.fromARGB(57, 13, 58, 131),
                          borderRadius: BorderRadius.circular(18)),
                      child: Padding(
                        padding: const EdgeInsets.only(left: 12.0),
                        child: TextField(
                          controller: _passwordController,
                          decoration: const InputDecoration(
                              border: InputBorder.none,
                              hintText: "Password",
                              hintStyle: TextStyle(color: Colors.white30)),
                          style: const TextStyle(color: Colors.white),
                          cursorColor: Colors.white,
                          obscureText: true,
                        ),
                      ),
                    ),
                    Expanded(
                      flex: 1,
                      child: Container(),
                    ),
                    GestureDetector(
                      onTap: changeWifi,
                      child: Container(
                        width: double.infinity,
                        decoration: BoxDecoration(
                            borderRadius: BorderRadius.circular(18),
                            color: const Color.fromARGB(255, 4, 49, 85)),
                        child: const Padding(
                          padding: EdgeInsets.all(18.0),
                          child: Text(
                            "Connect",
                            style: TextStyle(color: Colors.white, fontSize: 20),
                          ),
                        ),
                      ),
                    )
                  ],
                ),
              ),
            ),
          ),
        );
      },
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Row(
            children: [
              Icon(
                widget.rssi > -50
                    ? Icons.wifi_sharp
                    : widget.rssi > -90
                        ? Icons.wifi_2_bar
                        : Icons.wifi_1_bar,
                color: Colors.white,
              ),
              const SizedBox(
                width: 15,
              ),
              Text(
                widget.ssid,
                overflow: TextOverflow.ellipsis,
                style: const TextStyle(fontSize: 25, color: Colors.white),
              ),
            ],
          ),
          Icon(
            widget.authMode == "OPEN"
                ? Icons.lock_open_outlined
                : Icons.lock_outline_rounded,
            color: Colors.white,
            size: 25,
          )
        ],
      ),
    );
  }
}
