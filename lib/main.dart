import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:intl/intl.dart' as i;
import 'package:ordered_set/ordered_set.dart';

void main(List<String> args) {
  print(args);
  if (args.length >= 1) {
    return runApp(
      Container(
        color: Colors.red,
        child: GestureDetector(
          onTap: () => debugPrint("clicked"),
          child: Center(
            child: Container(
              width: 50,
              height: 50,
              color: Colors.pink,
              constraints: BoxConstraints.tight(const Size(50, 50)),
            ),
          ),
        ),
      ),
    );
  }
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
        colorScheme: const ColorScheme.light(
          surface: Colors.transparent,
          error: Color.fromRGBO(0xeb, 0x4d, 0x4b, 1), //0xeb4d4b
        ),
        textTheme: const TextTheme(
          bodySmall: TextStyle(fontSize: 12.5, fontWeight: FontWeight.w300),
          bodyMedium: TextStyle(),
          bodyLarge: TextStyle(),
        ).apply(
          bodyColor: const Color.fromRGBO(0xc2, 0xce, 0xee, 1),
          displayColor: const Color.fromRGBO(0xc2, 0xce, 0xee, 1),
          fontFamily: "JetBrainsMonoNerdFont",
        ),
      ),
      child: const Directionality(
        textDirection: TextDirection.ltr,
        child: Main(),
      ),
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
        border: const Border(
          bottom: BorderSide(color: Color.fromRGBO(50, 68, 114, 0.5), width: 3),
        ),
      ),
      padding: const EdgeInsets.all(2),
      child: const Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Expanded(
            child: Row(
              spacing: 5,
              children: [Workspace(), Flexible(child: Window())],
            ),
          ),
          Clock(),
          Expanded(
            child: Row(
              mainAxisAlignment: MainAxisAlignment.end,
              spacing: 5,
              children: [Tray()],
            ),
          ),
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
      stream: Stream.periodic(const Duration(seconds: 1)),
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

Future<Socket> hyprlandListen(void Function((String, String)) listen) async {
  // $XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket2.sock

  String address =
      "${Platform.environment['XDG_RUNTIME_DIR']}/hypr/${Platform.environment['HYPRLAND_INSTANCE_SIGNATURE']}/.socket2.sock";

  final host = InternetAddress(address, type: InternetAddressType.unix);
  final socket = await Socket.connect(host, 0);
  socket.listen(
    (data) {
      for (var event in utf8.decode(data).split('\n')) {
        final e = event.split(">>");
        if (e.firstOrNull == null) continue;
        if (e.first.isEmpty) continue;
        listen((e.first, e.lastOrNull ?? ''));
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
  return socket;
}

Future<T> hyprlandCommand<T>(List<String> commands) async {
  String address =
      "${Platform.environment['XDG_RUNTIME_DIR']}/hypr/${Platform.environment['HYPRLAND_INSTANCE_SIGNATURE']}/.socket.sock";

  final host = InternetAddress(address, type: InternetAddressType.unix);
  final socket = await Socket.connect(host, 0);

  socket.write("j/${commands.join(' ')}");

  final data = String.fromCharCodes(await socket.first);
  if (data == "unknown request") throw Exception("unknown request");
  await socket.close();
  if (T == String) return data as T;
  final jsonData = json.decode(data) as T;
  return jsonData;
}

mixin HyprlandListener {
  final streamController = StreamController<(String, String)>.broadcast();
  Socket? socket;

  void init() async {
    socket = await hyprlandListen(streamController.add);
  }

  void destroy() {
    socket?.destroy();
    streamController.close();
  }
}

class _WindowState extends State<Window> with HyprlandListener {
  @override
  void initState() {
    super.initState();
    init();
  }

  @override
  void dispose() {
    super.dispose();
    destroy();
  }

  Future<String?> _fetchInitialData() async {
    final data = await hyprlandCommand<Map<String, dynamic>>(["activewindow"]);
    return data['title'];
  }

  @override
  Widget build(BuildContext context) {
    final activeTitle = streamController.stream
        .where((event) => event.$1 == "activewindow")
        .map((event) => event.$2.split(',').lastOrNull ?? '');

    return FutureBuilder(
      future: _fetchInitialData(),
      builder: (context, snapshot) {
        if (!snapshot.hasData && snapshot.data == null) return const MyText('');
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

class Workspace extends StatefulWidget {
  const Workspace({super.key});

  @override
  State<Workspace> createState() => _WorkspaceState();
}

class _WorkspaceState extends State<Workspace> with HyprlandListener {
  @override
  void initState() {
    super.initState();
    init();
    setupStreamListeners();
    loadExistingWorkspaces();
  }

  void setupStreamListeners() {
    streamController.stream.listen((event) => debugPrint("$event"));

    streamController.stream
        .where((event) => event.$1 == 'createworkspacev2')
        .map((event) => int.parse(event.$2.split(',').first))
        .listen((e) => setState(() => orderedSet.add(e)));

    streamController.stream
        .where((event) => event.$1 == 'destroyworkspacev2')
        .map((event) => int.parse(event.$2.split(',').first))
        .listen((e) => setState(() => orderedSet.remove(e)));

    streamController.stream
        .where((event) => event.$1 == 'workspacev2')
        .map((event) => int.parse(event.$2.split(',').first))
        .listen((e) => setState(() => currentWorkspace = e));

    streamController.stream
        .where((event) => event.$1 == "urgent")
        .map((event) => int.parse("0x${event.$2}"))
        .listen(onUrgent);
  }

  void onUrgent(int a) async {
    final clients = await hyprlandCommand<List<dynamic>>(["clients"]);
    for (var client in clients) {
      final address = int.parse(client["address"]);
      if (a == address) {
        setState(() => urgentWorkspace = client["workspace"]["id"]);
        break;
      }
      debugPrint("$a $address");
    }
  }

  int currentWorkspace = -1;
  final orderedSet = OrderedSet<int>();

  int urgentWorkspace = -1;

  void onWorkspaceClicked(int id) async {
    if (id == urgentWorkspace) setState(() => urgentWorkspace = -1);
    await hyprlandCommand<String>(["dispatch", "workspace", "$id"]);
  }

  void loadExistingWorkspaces() async {
    final workspaces = await hyprlandCommand<List<dynamic>>(["workspaces"]);
    for (var i in workspaces) {
      // debugPrint(i.toString());
      orderedSet.add(i['id']);
    }

    final activeWorkspace = await hyprlandCommand<Map<String, dynamic>>([
      "activeworkspace",
    ]);
    currentWorkspace = activeWorkspace["id"];
    setState(() {});
  }

  @override
  void dispose() {
    super.dispose();
    destroy();
  }

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        for (var id in orderedSet)
          NewWidget(
            "$id",
            isActive: currentWorkspace == id,
            isAttentionRequired: urgentWorkspace == id,
            onClick: () => onWorkspaceClicked(id),
          ),
      ],
    );
  }
}

class NewWidget extends StatefulWidget {
  const NewWidget(
    this.text, {
    super.key,
    this.isActive = false,
    this.isAttentionRequired = false,
    this.onClick,
  });

  final bool isActive;
  final bool isAttentionRequired;
  final String text;
  final void Function()? onClick;
  @override
  State<NewWidget> createState() => _NewWidgetState();
}

class _NewWidgetState extends State<NewWidget> {
  bool haveHover = false;
  @override
  Widget build(BuildContext context) {
    return MouseRegion(
      onEnter: (event) => setState(() => haveHover = true),
      onExit: (event) => setState(() => haveHover = false),
      child: GestureDetector(
        onTap: widget.onClick,
        child: Container(
          padding: const EdgeInsets.only(left: 5, right: 5),
          decoration: BoxDecoration(
            border: Border(
              bottom: BorderSide(
                color:
                    haveHover
                        ? const Color.fromARGB(0xFF, 0x70, 0xAA, 0xD5)
                        : Colors.transparent,
                width: 2.5,
              ),
            ),
            color:
                widget.isAttentionRequired
                    ? Theme.of(context).colorScheme.error
                    : Colors.transparent,
          ),
          child:
              widget.isActive
                  ? Text(
                    widget.text,
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      overflow: TextOverflow.ellipsis,
                      fontWeight: FontWeight.w400,
                    ),
                  )
                  : MyText(widget.text),
        ),
      ),
    );
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

class Margin {
  final double left;
  final double top;
  final double right;
  final double bottom;
  const Margin({this.left = 0, this.right = 0, this.bottom = 0, this.top = 0});

  Map<String, dynamic> toMap() {
    return {'left': left, 'top': top, 'right': right, 'bottom': bottom};
  }

  String toJson() => json.encode(toMap());
}

enum Position { left, topcenter, right }

class Params {
  final int width;
  final int height;
  final Position position;
  final Margin margin;

  const Params({
    required this.width,
    required this.height,
    this.position = Position.topcenter,
    this.margin = const Margin(),
  });

  Map<String, dynamic> toMap() {
    return {
      'width': width,
      'height': height,
      'position': position.index,
      'margin': margin.toMap(),
    };
  }

  String toJson() => json.encode(toMap());
}

class Tray extends StatefulWidget {
  const Tray({super.key});

  @override
  State<Tray> createState() => _TrayState();
}

class _TrayState extends State<Tray> {
  final channel = const MethodChannel('internal.window.manager');

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      child: Container(width: 20, color: Colors.red,),
      onTap:
          () => channel.invokeMethod(
            'new_window',
            const Params(
              height: 200,
              width: 200,
              position: Position.right,
              margin: Margin(right: 2),
            ).toJson(),
          ),
    );
  }
}

class Notification extends StatelessWidget {
  const Notification({super.key});

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
  }
}
