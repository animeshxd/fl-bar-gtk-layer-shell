import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:intl/intl.dart' as i;

void main() {
  runApp(const App());
}

class App extends StatelessWidget {
  const App({super.key});

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return Theme(
      data: ThemeData(
        fontFamily: "JetBrainsMonoNerdFont",
        colorScheme: ColorScheme.light(surface: Colors.transparent),
        textTheme: TextTheme(
          bodySmall: TextStyle(fontSize: 12.5, fontWeight: FontWeight.w300),
          bodyMedium: TextStyle(),
          bodyLarge: TextStyle(),
        ).apply(
          bodyColor: Color.fromRGBO(0xc2, 0xce, 0xee, 1),
          displayColor: Color.fromRGBO(0xc2, 0xce, 0xee, 1),
          fontFamily: "JetBrainsMonoNerdFont",
        ),
      ),
      child: Directionality(textDirection: TextDirection.ltr, child: Main()),
    );
  }
}

class Main extends StatelessWidget {
  const Main({super.key});

  @override
  Widget build(BuildContext context) {
    return Container(
      height: MediaQuery.sizeOf(context).height,
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        border: Border(
          bottom: BorderSide(color: Color.fromRGBO(50, 68, 114, 0.5), width: 3),
        ),
      ),
      padding: EdgeInsets.all(2),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Expanded(child: Row(children: [Flexible(child: Window())])),
          Clock(),
          Expanded(child: Container()),
        ],
      ),
    );
  }
}

class Clock extends StatelessWidget {
  const Clock({super.key});

  @override
  Widget build(BuildContext context) {
    return StreamBuilder(
      stream: Stream.periodic(Duration(seconds: 1)),
      builder: (context, snapshot) {
        return MyText(i.DateFormat("hh:mm a").format(DateTime.now()));
      },
    );
  }
}

class Window extends StatefulWidget {
  const Window({super.key});

  @override
  State<Window> createState() => _WindowState();
}

Future<void> hyprlandListen(void Function((String, String)) listen) async {
  // $XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket2.sock

  String address =
      "${Platform.environment['XDG_RUNTIME_DIR']}/hypr/${Platform.environment['HYPRLAND_INSTANCE_SIGNATURE']}/.socket2.sock";

  final host = InternetAddress(address, type: InternetAddressType.unix);
  final socket = await Socket.connect(host, 0);
  socket.listen(
    (data) {
      for (var event in utf8.decode(data).split('\n')) {
        final e = event.split(">>");
        if (e.firstOrNull != null && e.lastOrNull != null) {
          debugPrint(e.toString());
          listen((e.first, e.last));
        }
      }
    },
    onError: (error) {
      // Handle connection error
      debugPrint('Connection error: $error');
    },
    onDone: () {
      // Handle socket closure
      debugPrint('Socket closed');
      socket.close();
    },
  );
}

Future<String> hyprlandCommand(List<String> commands) async {
  String address =
      "${Platform.environment['XDG_RUNTIME_DIR']}/hypr/${Platform.environment['HYPRLAND_INSTANCE_SIGNATURE']}/.socket.sock";

  final host = InternetAddress(address, type: InternetAddressType.unix);
  final socket = await Socket.connect(host, 0);

  socket.write("j/${commands.join(' ')}");

  final data = String.fromCharCodes(await socket.first);
  if (data == "unknown request") throw Exception("unknown request");
  await socket.close();
  return data;
}

class _WindowState extends State<Window> {
  final streamController = StreamController<(String, String)>.broadcast();
  @override
  void initState() {
    super.initState();
    hyprlandListen(streamController.add);
  }

  Future<String?> _fetchInitialData() async {
    final data = await hyprlandCommand(["activewindow"]);
    final jsonData = json.decode(data) as Map<String, dynamic>;
    return jsonData['title'];
  }

  @override
  Widget build(BuildContext context) {
    final activeTitle = streamController.stream
        .where((event) => event.$1 == "activewindow")
        .map((event) => event.$2.split(',').lastOrNull ?? '');

    return FutureBuilder(
      future: _fetchInitialData(),
      builder: (context, snapshot) {
        if (!snapshot.hasData && snapshot.data == null) return MyText('');
        return StreamBuilder(
          stream: activeTitle,
          initialData: snapshot.data,
          builder: (context, snapshot) => MyText(snapshot.data ?? ''),
        );
      },
    );
  }
}

class MyText extends StatelessWidget {
  const MyText(this.text, {super.key});
  final String text;

  @override
  Widget build(BuildContext context) {
    return Text(
      text,
      style: Theme.of(
        context,
      ).textTheme.bodySmall?.copyWith(overflow: TextOverflow.ellipsis),
    );
  }
}

class Workspace extends StatelessWidget {
  const Workspace({super.key});

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
  }
}

class Network extends StatelessWidget {
  const Network({super.key});

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
  }
}

class PulseAudio extends StatelessWidget {
  const PulseAudio({super.key});

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
  }
}

class Tray extends StatelessWidget {
  const Tray({super.key});

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
  }
}

class Notification extends StatelessWidget {
  const Notification({super.key});

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
  }
}
