# 5. State Machine

- Boot → Init → Idle
- Idle —[SW2 short]→ Recording
- Recording —[SW2 short | LowBatt | Error]→ Stopping → Idle
- Idle —[Timeout]→ Light Sleep —[SW2/Timer]→ Idle
- Extended inactivity → Deep Sleep —[SW2]→ Boot
