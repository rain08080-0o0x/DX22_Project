# Work Notes - DX22_Project copy

Context
- Project root: folder containing this memo.
- Goal: add HP UI and make turn damage apply based on bet.

Changes
- SceneGame.cpp
  - Cache role on dice stop and apply result once per stop.
  - ApplyBetResult sets m_damageThisTurn and calls EndTurn on round end.
  - Enemy turn uses fixed bet (5) so damage is visible.
  - HP UI created/drawn; EndTurn updates HP UI.
  - Money only increases on player win.
- SceneGame.h
  - Added m_pPlayerHp / m_pEnemyHp members.

Build
- MSBuild path:
  "C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
- Command (run from project root):
  MSBuild.exe "DX22_Project.sln" /t:Build /p:Configuration=Debug /p:Platform=x64
- Output exe:
  .\x64\Debug\DX22_Project.exe

Notes
- If build fails with LNK1168, close the running DX22_Project.exe and rebuild.

Next checks
- Run the exe and confirm HP numbers change on win/lose.
- Verify turn indicator switches between Player/Enemy and enemy auto-rolls.
