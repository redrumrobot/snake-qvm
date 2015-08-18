# Introduction #

My QVM have been specially made to make tremulous more enjoyable without balance changes (1.1 gameplay)


# Features #

  * **Automatic Decon Reverter and kicker:** when you use !decon playername, it will automatically revert this player's decon and kick him for the amount of time specified in g\_deconBanTime (default is 1w), the reason is ^1Decon.

  * **Easier Restarts:** now you don't have to type the whole thing to restart with options, only type !restart stl for switchteamslock, !restart ktl for keepteamslock, !restart kt for keepteams and !restart st for switchteams, easy, isnt it?

  * **Both Teams Lock:** This was made to avoid the annoying players joining to the team which is not locked, also to only use 1 cmd to lock both teams, use !lock e to lock both teams and !unlock e to unlock both teams

  * **Names Log:** Logs the name, the IP and the guid of all the players who enter into the server (It was made to keep a track of the aliases), Managed by the g\_NamesFile cvar

  * **Abusable CMDs:** I've groupped the abusable CMDs and made a cvar (g\_noAbusableCMDs), when enabled(1) it will disable all the abusable cmds(cookie, drop, slap, abps, hbps, astage, hstage, bps, stage).

  * **No Register Levels:** This was made to keep a track of your admins, when their level is between the g\_minNoRegisterLevel and the g\_maxNoRegister level, it wont let they register their new aliases
(Example: when g\_NoRegisterLevels is 1, g\_minNoRegisterLevel is 2 and g\_maxNoRegister level is 6, the admins that are a level between 2 and 6 wont be able to register

  * **Report files:** !report command allows your players to report an special situation on the server, comes along with the current !namelog (Originally wrote by Amanieu and/or Benmachine?), managed by the cvars g\_reportFile and g\_reportLimit

  * **Invisible:** Now admins are capable of being invisible to the players and other admins, I used the patch made by Citrux, Slacker and Tori but fixed a lot of bugs that could notice the players of the real status of the admin, however if you use admin as invisible they will notice of you easily (or think you are "hacking" the server :P). It still its kinda buggy but I'll be fixing it with the time, Its safe to use.

  * **Admin Reports:** It streams all the commands used by all the admins to the adminchat, it was made to spot the admin abusers on-the-fly, it reports all the commands except for: adminlog, admintest, listmaps, help, info, listadmins, listplayers, rotation, drop, specme, cookie, fortunecookie, fireworks, invisible, namelog, showbans and time, also it won't report the commands usage if the admin is invisible.

  * **Auto TK warn and kick:** This feature is broken, it's supposed to auto warn and autokick the teamkillers after a number of tks but for some reason it haven't been working properly so I 'disabled' it, I'll give news when I fix it or something, its managed by the cvars g\_maxTKs and g\_maxTKtime, default are disabled because of broken, DON'T DARE YOU TO SWITCH IT, IT COULD KICK YOU FROM 1 TK.

  * **Multi Char Flags:** Multi character flags support, also added .NOVOTE and .NOCHAT when you give a player (or a whole level if you are mean :P) these flags they can't call votes or chat (while having the flag) respectively //Multichar flags patch was made by Rezyn

  * **Website:** the website cvar was made with the objective of tell the players where to do some things, a PoC of this is !amIadmin which tells you if you are an admin, and if not tells you where to apply, setting the g\_website will let you make use of that cvar with different usages. It's still under development but you should setup already g\_website so you won't have any trouble in the future.

  * **Stage and round commands:** I added the next CMDs with the point of helping the clans to train or just have fun: astage, hstage, stage, abps, hbps, bps; these are usable only if the round is DEVMAP or g\_noAbusableCMDs is 0