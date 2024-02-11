import 'package:flutter/material.dart';

import 'services/services.dart';
import 'models/wifi_model.dart';
import 'widgets/wifi_showcase.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple)
            .copyWith(background: const Color(0xff121421)),
      ),
      home: const MyHomePage(),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key});

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  List<WiFi>? wifi;
  List<WiFiStatus>? status;
  late bool isSwitched = false;
  final ApiServices apiServices = ApiServices();

  @override
  void initState() {
    super.initState();
    fetchWifi();
  }

  void getWifiStatus() async {
    status = await apiServices.getWifiStatus(context: context);
    isSwitched = status![0].active;
    setState(() {});
  }

  void fetchWifi() async {
    getWifiStatus();
    wifi = await apiServices.scanWifi(context: context);
    setState(() {});
  }

  void toggleSwitch(bool value) {
    if (isSwitched == false) {
      apiServices.changeMode(context: context, active: "1");
      setState(() {
        fetchWifi();
        isSwitched = true;
      });
      print('Switch Button is ON');
    } else {
      apiServices.changeMode(context: context, active: "0");
      setState(() {
        isSwitched = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      resizeToAvoidBottomInset: true,
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(18.0),
          child: Column(
            children: [
              const Row(
                children: [
                  Text(
                    "Esp WiFi",
                    style: TextStyle(
                        color: Colors.white,
                        fontSize: 65,
                        fontWeight: FontWeight.bold),
                  ),
                ],
              ),
              const SizedBox(
                height: 20,
              ),
              Padding(
                padding: const EdgeInsets.all(8.0),
                child: Container(
                  height: 100,
                  width: double.infinity,
                  decoration: BoxDecoration(
                      borderRadius: BorderRadius.circular(10),
                      color: Colors.pink),
                  child: Padding(
                    padding: const EdgeInsets.all(8.0),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        const Text(
                          "   Wifi",
                          style: TextStyle(color: Colors.white, fontSize: 35),
                        ),
                        Switch(
                          value: isSwitched,
                          onChanged: toggleSwitch,
                          activeColor: Colors.white,
                          activeTrackColor:
                              const Color.fromARGB(255, 4, 49, 85),
                          inactiveThumbColor: Colors.white,
                          inactiveTrackColor:
                              const Color.fromARGB(255, 4, 49, 85),
                        )
                      ],
                    ),
                  ),
                ),
              ),
              const SizedBox(
                height: 30,
              ),
              Container(
                height: 2,
                color: Colors.white60,
              ),
              const SizedBox(
                height: 15,
              ),
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Text(
                    "Avaliable Networks",
                    style: TextStyle(color: Colors.white, fontSize: 20),
                  ),
                  GestureDetector(
                    onTap: fetchWifi,
                    child: const Icon(
                      Icons.refresh,
                      color: Colors.white,
                    ),
                  )
                ],
              ),
              isSwitched
                  ? Padding(
                      padding: const EdgeInsets.only(top: 18.0),
                      child: wifi == null
                          ? const Center(
                              child: CircularProgressIndicator(
                              color: Colors.pink,
                            ))
                          : SizedBox(
                              height: MediaQuery.of(context).size.height - 415,
                              child: ListView.builder(
                                scrollDirection: Axis.vertical,
                                itemCount: wifi!.length,
                                itemBuilder: (context, index) {
                                  return Padding(
                                    padding: const EdgeInsets.all(8.0),
                                    child: Container(
                                      decoration: BoxDecoration(
                                          color: status![0].ssid ==
                                                  wifi![index].ssid
                                              ? Colors.purple
                                              : Colors.pink,
                                          borderRadius:
                                              BorderRadius.circular(20)),
                                      width: double.infinity,
                                      height: 115,
                                      child: Padding(
                                        padding: const EdgeInsets.all(18.0),
                                        child: WifiWidget(
                                          ssid: wifi![index].ssid,
                                          authMode: wifi![index].authMode,
                                          rssi: wifi![index].rssi,
                                        ),
                                      ),
                                    ),
                                  );
                                },
                              ),
                            ),
                    )
                  : const Expanded(
                      flex: 1,
                      child: Center(
                        child: Text(
                          "Turn On Wifi",
                          style: TextStyle(color: Colors.white, fontSize: 25),
                        ),
                      ),
                    ),
            ],
          ),
        ),
      ),
    );
  }
}
