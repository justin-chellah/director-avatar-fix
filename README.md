# [L4D2] Director Avatar Fix
This is a SourceMod Extension designed for **Versus or PvP** that fixes various issues related to player avatars. Whenever players join a game from the lobby after selecting their character (avatar), the Director stores this information on the server in order to keep track of what characters players have. For example, this is used so that when a level changes, players will play as the characters they've picked from the lobby but characters can also be auto-assigned when joining a game without having been in a lobby before.

This extension should not be used if you're running plugins or extensions that override the team limits (4+ players).

# Fixed Issues
```
- New joining players will be assigned to teams without considering other loading players which can lead to losing spots
- There would be no avatar information stored on the server if the server is unreserved
- Players that joined but weren't able to join any team (survivor or infected) would be sent to spectators but avatar information wouldn't be updated
- Players won't be sent to the right teams if they join the game while teams have been flipped if their destination team is full
- Players won't be sent to the right teams after level transitions if they join the game while their spots have been taken by other players
- Avatar information wouldn't update properly on team changes which will cause survivors to lose their desired character after level transitions when teams aren't flipped
- The Director would store L4D1 avatar information which would prevent loading survivors to play as their desired character if all survivor bots have already joined on L4D2 maps
- The Director wouldn't respect survivor bots that are reserved for other loading players when randomly assigning a bot to a player
- Players that would join the server through matchmaking could cause others to lose their spots if they load faster than them, after level transitions
- When returning back to the lobby, players would suddenly no longer have their characters selected
```

# Requirements
- [SourceMod 1.11+](https://www.sourcemod.net/downloads.php?branch=stable)
- [Matchmaking Extension Interface](https://github.com/shqke/imatchext)

# Supported Games
- Left 4 Dead 2

# Supported Platforms
- Windows
- Linux