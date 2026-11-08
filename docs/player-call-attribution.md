# Player-call CPU attribution

The bounded player-call interpreter in `src/call.cpp` adapts CPU operations from cRSID by Mihály 'Hermit' Horváth, distributed with SID-Wizard. The original source permits reuse with acknowledgement of its author. Its permission statement reads: “License: WTF - do what the frick you want with this code, but please mention me as the original author”. The original author and source are acknowledged in the implementation header.

The implementation was adapted for bounded execution, guarded memory banking, supported instruction validation, write recording and transactional installation of results. It is used only by the optional recognised-player seek shortcut; ordinary C64 execution remains in the cycle-based CPU.
