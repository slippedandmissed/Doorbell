import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:doorbell/firebase_options.dart';
import 'package:doorbell/home.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_messaging/firebase_messaging.dart';
import 'package:flutter/material.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(options: DefaultFirebaseOptions.currentPlatform);
  await FirebaseMessaging.instance.requestPermission();
  final fcmToken = await FirebaseMessaging.instance.getToken();
  debugPrint(fcmToken);
  final firestore = FirebaseFirestore.instance;
  final docs = await firestore
      .collection("fcmTokens")
      .where("fcmToken", isEqualTo: fcmToken)
      .get();
  if (docs.size == 0) {
    await firestore.collection("fcmTokens").add({"fcmToken": fcmToken});
  }
  runApp(const App());
}

class App extends StatelessWidget {
  const App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Doorbell',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      debugShowCheckedModeBanner: false,
      home: const HomePage(),
    );
  }
}
