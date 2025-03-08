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
        colorScheme: ColorScheme.light(surface: Color.fromRGBO(14, 14, 26, 1)),
        textTheme: TextTheme(
          bodySmall: TextStyle(fontSize: 13, fontWeight: FontWeight.w300),
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
          Expanded(child: Container()),
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
        return Text(
          i.DateFormat("hh:mm a").format(DateTime.now()),
          style: Theme.of(context).textTheme.bodySmall,
        );
      },
    );
  }
}

class Window extends StatelessWidget {
  const Window({super.key});

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
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