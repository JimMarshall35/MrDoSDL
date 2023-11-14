# MrDoSDL
A version of the 1982 arcade game Mr Do!
Special features:
  - custom scripting language interpreter / compiler used to script enemy behavior (based on forth - see https://github.com/JimMarshall35/Forth for the main project this code is copied from which includes test suite)
  - replay recording
  - back end server with unique property - to submit a high score you submit the replay file - it is played back on the backend version by a headless version of the game with uncapped framerate and the resulting score compared to the one submitted. This process takes place very quickly and is not really noticable to the user. This should form a pretty effective anti cheat system as in order to submit a fake high score you would have to also craft a fake replay file that results in that high score. The downside is the version of the replay validator app must match the version of the game and version info must be encoded in the replay file header and the possibility of incompatibility handled.
  - replay validator on back end compiled from same code as the game itself, ReplayValidator define causes the replay validator command line app to be compiled instead of the game
