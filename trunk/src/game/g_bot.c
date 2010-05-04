/*
===========================================================================
Copyright (C) 2007 Amine Haddad

This file is part of Tremulous.

The original works of vcxzet (lamebot3) were used a guide to create TremBot.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/* Current version: v0.01 */

#include "g_local.h"

#ifndef RAND_MAX
#define RAND_MAX 32768
#endif
void G_BotReload( gentity_t *ent, int clientNum )
{
  ClientDisconnect( clientNum );
  G_BotAdd( ent->client->pers.netname, ent->client->pers.teamSelection, ent->botSkillLevel, clientNum );
  trap_SendServerCommand( -1, "print \"Interfering bot reloaded\n\"" );
}

void G_DeleteBots( void )
{
  int i;
  gentity_t *bot;
  bot = &g_entities[ 0 ];
  for( i = 0; i < level.maxclients; i++, bot++ )
  {
    if(bot->r.svFlags & SVF_BOT) 
    {ClientDisconnect(bot->client->ps.clientNum);}
  }
}

void G_BotAdd( char *name, int team, int skill, int ignore ) {
  int i,i2;
  int clientNum;
  char userinfo[MAX_INFO_STRING];
  int reservedSlots = 0;
  gentity_t *bot;

  reservedSlots = trap_Cvar_VariableIntegerValue( "sv_privateclients" );

  // find what clientNum to use for bot
  clientNum = -1;
  for( i = 0; i < reservedSlots; i++ ) {
    if( i == ignore )
    {continue;}
    if( !g_entities[i].inuse ) {
      clientNum = i;
      break;
    }
  }
  if(clientNum == 0)
  {
    trap_Printf("Warning: Bots will be reloaded if they interfere with client connections\n");
  }
  if(clientNum < 0) {
    trap_Printf("no more slots for bot\n");
    return;
  }

  bot = &g_entities[ clientNum ];
  bot->inuse = qtrue;

  //default bot data
  bot->botCommand = BOT_REGULAR;
  bot->botFriend = NULL;
  bot->botEnemy = NULL;
  bot->botFriendLastSeen = 0;
  bot->botEnemyLastSeen = 0;
  if(skill > 4){skill = 4;}
  if(skill < 0){skill = 0;}
  bot->botSkillLevel = skill;
  bot->botTeam = team;
  bot->pathChosen = qfalse;
  bot->nextNode = qfalse;
  bot->numOldPaths = 0;
  bot->state = FINDNEWPATH;
  bot->blocked = qfalse;

  for(i2 = 0;i2 < MAX_PATHS;i2++)
  {
    bot->OldPaths[i2].nextid[0] = -1;
    bot->OldPaths[i2].nextid[1] = -1;
    bot->OldPaths[i2].nextid[2] = -1;
    bot->OldPaths[i2].nextid[3] = -1;
    bot->OldPaths[i2].nextid[4] = -1;
    bot->SkipPaths[i2] = -1;
  }
  // register user information
  userinfo[0] = '\0';
  Info_SetValueForKey( userinfo, "name", name );
  Info_SetValueForKey( userinfo, "rate", "25000" );
  Info_SetValueForKey( userinfo, "snaps", "20" );
  Info_SetValueForKey( userinfo, "ip", "1.2.3.4" );
  if( *g_password.string && Q_stricmp( g_password.string, "none" ) )
    Info_SetValueForKey( userinfo, "password", g_password.string );

  trap_SetUserinfo( clientNum, userinfo );

  // have it connect to the game as a normal client
  if(ClientConnect(clientNum, qtrue) != NULL ) {
    // won't let us join
    return;
  }
  bot->r.svFlags |= SVF_BOT;
  ClientBegin( clientNum );
  G_ChangeTeam( bot, team );
}

void G_BotDel( int clientNum ) {
  gentity_t *bot;

  bot = &g_entities[clientNum];
  if( !( bot->r.svFlags & SVF_BOT ) ) {
    trap_Printf( va("'^7%s^7' is not a bot\n", bot->client->pers.netname) );
    return;
  }
  bot->inuse = qfalse;
  ClientDisconnect(clientNum);
}

void G_BotCmd( gentity_t *master, int clientNum, char *command ) {
  gentity_t *bot;

  bot = &g_entities[clientNum];
  if( !( bot->r.svFlags & SVF_BOT ) ) {
    return;
  }

  bot->botFriend = NULL;
  bot->botEnemy = NULL;
  bot->botFriendLastSeen = 0;
  bot->botEnemyLastSeen = 0;

  if( !Q_stricmp( command, "regular" ) ) {
    bot->botCommand = BOT_REGULAR;
    //trap_SendServerCommand(-1, "print \"regular mode\n\"");
  } else if( !Q_stricmp( command, "idle" ) ) {
    bot->botCommand = BOT_IDLE;
    //trap_SendServerCommand(-1, "print \"idle mode\n\"");
  } else if( !Q_stricmp( command, "attack" ) ) {
    bot->botCommand = BOT_ATTACK;
    //trap_SendServerCommand(-1, "print \"attack mode\n\"");
  } else if( !Q_stricmp( command, "standground" ) ) {
    bot->botCommand = BOT_STAND_GROUND;
    //trap_SendServerCommand(-1, "print \"stand ground mode\n\"");
  } else if( !Q_stricmp( command, "defensive" ) ) {
    bot->botCommand = BOT_DEFENSIVE;
    //trap_SendServerCommand(-1, "print \"defensive mode\n\"");
  } else if( !Q_stricmp( command, "followprotect" ) ) {
    bot->botCommand = BOT_FOLLOW_FRIEND_PROTECT;
    bot->botFriend = master;
    //trap_SendServerCommand(-1, "print \"follow-protect mode\n\"");
  } else if( !Q_stricmp( command, "followattack" ) ) {
    bot->botCommand = BOT_FOLLOW_FRIEND_ATTACK;
    bot->botFriend = master;
    //trap_SendServerCommand(-1, "print \"follow-attack mode\n\"");
  } else if( !Q_stricmp( command, "followidle" ) ) {
    bot->botCommand = BOT_FOLLOW_FRIEND_IDLE;
    bot->botFriend = master;
    //trap_SendServerCommand(-1, "print \"follow-idle mode\n\"");
  } else if( !Q_stricmp( command, "teamkill" ) ) {
    bot->botCommand = BOT_TEAM_KILLER;
    //trap_SendServerCommand(-1, "print \"team kill mode\n\"");
  } else {
    bot->botCommand = BOT_REGULAR;
    //trap_SendServerCommand(-1, "print \"regular (unknown) mode\n\"");
  }

  return;
}

int distanceToTargetNode( gentity_t *self )
{
  int distance,Ax,Ay,Az,Bx,By,Bz = 0;
  Ax = level.paths[self->targetPath].coord[0];
  Ay = level.paths[self->targetPath].coord[1];
  Az = level.paths[self->targetPath].coord[2];
  Bx = self->s.pos.trBase[0];
  By = self->s.pos.trBase[1];
  Bz = self->s.pos.trBase[2];
  distance = sqrt((Ax - Bx)*(Ax - Bx) + (Ay - By)*(Ay - By) + (Az - Bz)*(Az - Bz));
  return distance;
}

qboolean botAimAtPath( gentity_t *self )
{
  vec3_t dirToTarget, angleToTarget;
  vec3_t top = { 0, 0, 0};
  int vh = 0;
  BG_FindViewheightForClass(  self->client->ps.stats[ STAT_PCLASS ], &vh, NULL );
  top[2]=vh;
  VectorAdd( self->s.pos.trBase, top, top);
  VectorSubtract( level.paths[self->targetPath].coord, top, dirToTarget );
  VectorNormalize( dirToTarget );
  vectoangles( dirToTarget, angleToTarget );
  self->client->ps.delta_angles[ 0 ] = ANGLE2SHORT( angleToTarget[ 0 ] );
  self->client->ps.delta_angles[ 1 ] = ANGLE2SHORT( angleToTarget[ 1 ] );
  self->client->ps.delta_angles[ 2 ] = ANGLE2SHORT( angleToTarget[ 2 ] );
  return qtrue;
}

void G_FrameAim( gentity_t *self )
{
  if( !( self->r.svFlags & SVF_BOT ) )
  {
    return;
  }
  if(self->botEnemy && self->botEnemy->health <= 0)
    self->botEnemy = NULL;
  else
  {
    if(botTargetInRange( self, self->botEnemy ) == qfalse)
    {
      self->botEnemy = NULL;
    }
  }
  if(self->botFriend && self->botFriend->health <= 0)
    self->botFriend = NULL;
  else
  {
    if(botTargetInRange( self, self->botFriend ) == qfalse)
    {
      self->botFriend = NULL;
    }
  }
  if(self->botFriend->health <= 0)
    self->botFriend = NULL;
  if(!self->botEnemy && !self->botFriend && self->state == TARGETPATH)
  {
    botAimAtPath(self);
  }
  else if(self->botEnemy)
  {
    botAimAtTarget(self, self->botEnemy);
  }
  else if(self->botFriend)
  {
    botAimAtTarget(self, self->botFriend);
  }
}

void G_FastThink( gentity_t *self )
{
  int forwardMove = 127;
  if( !( self->r.svFlags & SVF_BOT ) )
  {
    return;
  }
  if(self->botEnemy || self->botFriend)
  {
    self->enemytime = level.time;
  }
  if(!self->botEnemy && !self->botFriend && self->state == TARGETPATH)
  {
    self->client->pers.cmd.buttons = 0;
    self->client->pers.cmd.upmove = 0;
    self->client->pers.cmd.rightmove = 0;
    if((self->isblocked == qtrue || 
        VectorLength( self->client->ps.velocity ) < 50.0f) && level.time - self->enemytime > 1000)
    {
      self->client->pers.cmd.buttons |= BUTTON_GESTURE;
      self->client->pers.cmd.rightmove = -100;
      if(self->client->time1000 >= 500)
      {
        self->client->pers.cmd.rightmove = 100;
      }
      if(level.time - self->jumptime > 3000 &&
         (( self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS && self->client->ps.stats[ STAT_STAMINA ] > 100 ) ||
         self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS))
      {
        self->client->pers.cmd.upmove = 20;
        if(level.time - self->jumptime > 4000)
          self->jumptime = level.time;
      }
    }
    self->client->pers.cmd.forwardmove = forwardMove;
    if(self->lastpathid >= 0)
    {
      switch(level.paths[self->lastpathid].action)
      {
        case BOT_JUMP:	if(level.time - self->jumptime > 3000)
        {
          self->client->pers.cmd.upmove = 20;
          if(level.time - self->jumptime > 4000)
            self->jumptime = level.time;
        }
        break;
        case BOT_WALLCLIMB: if( BG_ClassHasAbility( self->client->ps.stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) ) {
          self->client->pers.cmd.upmove = -1;
        }
        break;
        case BOT_KNEEL: if(self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS)
        {
          self->client->pers.cmd.upmove = -1;
        }
        break;
        case BOT_POUNCE:if(self->client->pers.classSelection == PCL_ALIEN_LEVEL3 && 
            self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_SPEED)
                self->client->pers.cmd.buttons |= BUTTON_ATTACK2;
            else if(self->client->pers.classSelection == PCL_ALIEN_LEVEL3_UPG && 
                    self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_UPG_SPEED)
              self->client->pers.cmd.buttons |= BUTTON_ATTACK2;
            break;
            default: break;
      }
      if(level.time - self->timeFoundPath > level.paths[self->lastpathid].timeout)
      {
        self->state = FINDNEWPATH;
        self->timeFoundPath = level.time;
        self->SkipPaths[self->targetPath] = 1;
      }
    }
    else if( level.time - self->timeFoundPath > 10000 )
    {
      self->state = FINDNEWPATH;
      self->timeFoundPath = level.time;
      if(self->targetPath > -1)
        self->SkipPaths[self->targetPath] = 1;
    }
    if(distanceToTargetNode(self) < 70)
    {
      self->state = FINDNEXTPATH;
      self->timeFoundPath = level.time;
    }
    if(self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS && 
       self->client->ps.stats[ STAT_STAMINA ] < MAX_STAMINA)
    {
      self->client->pers.cmd.upmove = -1;
      self->client->pers.cmd.buttons = 0;
      self->client->pers.cmd.rightmove = 0;
      self->client->pers.cmd.forwardmove = 0;
      self->enemytime = level.time;
      self->timeFoundPath += 100;
    }
  }
        //else if(!self->botEnemy && !self->botFriend && self->state == LOST)
        //{
                //Run around wild?
        //}
}

void findnewpath( gentity_t *self )
{
  trace_t trace;
  int i,distance,Ax,Ay,Az,Bx,By,Bz = 0;
  int closestpath = 0;
  int closestpathdistance = 2000;
  qboolean pathfound = qfalse;
  for(i = 0; i < level.numPaths; i++) //find a nearby path that wasn't used before
  {
    trap_Trace( &trace, self->s.pos.trBase, NULL, NULL, level.paths[i].coord, self->s.number, MASK_SHOT );
    if( trace.fraction < 1.0 )
    {continue;}
    Ax = level.paths[i].coord[0];
    Ay = level.paths[i].coord[1];
    Az = level.paths[i].coord[2];
    Bx = self->s.pos.trBase[0];
    By = self->s.pos.trBase[1];
    Bz = self->s.pos.trBase[2];

                //for(i2 = 0;i2 < 5;i2++)
                //{
                        //if(self->OldPaths[i].nextid[i2] == level.paths[i].nextid[i2] || self->SkipPaths[i] > 0)
                        //{
                                //oldpath = qtrue;
                        //}
                        //else
                        //{
                                //oldpath = qfalse;
                                //i2 = 5;
                        //}
                //}
                //if(oldpath == qtrue){oldpath = qfalse; continue;}
    if(self->SkipPaths[i] > 0){continue;}
    distance = sqrt((Ax - Bx)*(Ax - Bx) + (Ay - By)*(Ay - By) + (Az - Bz)*(Az - Bz));
    if(distance < 5000)
    {
      if(closestpathdistance > distance)
      {
        closestpath = i;
        closestpathdistance = distance;
        pathfound = qtrue;
      }
    }
  }
  if(pathfound == qtrue)
  {
    self->targetPath = closestpath;
    self->timeFoundPath = level.time;
    self->state = TARGETPATH;
    self->isblocked = qfalse;
  }
  else
  {
    self->state = LOST;
    self->client->pers.cmd.forwardmove = 0;
    self->client->pers.cmd.upmove = -1;
    self->client->pers.cmd.rightmove = 0;
    self->client->pers.cmd.buttons = 0;
    self->client->pers.cmd.buttons |= BUTTON_GESTURE;
    for(i = 0;i < level.numPaths; i++)
    {
      self->OldPaths[i].nextid[0] = -1;
      self->OldPaths[i].nextid[1] = -1;
      self->OldPaths[i].nextid[2] = -1;
      self->OldPaths[i].nextid[3] = -1;
      self->OldPaths[i].nextid[4] = -1;
      self->SkipPaths[i] = -1;
    }
  }
  return;
}

void findnextpath( gentity_t *self )
{
  int randnum = 0;
  int i,nextpath = 0;
  int possiblenextpath = 0;
  int possiblepaths[5];
  qboolean oldpath = qfalse;
  int lasttarget = self->targetPath;
  possiblepaths[0] = possiblepaths[1] = possiblepaths[2] = possiblepaths[3] = possiblepaths[4] = 0;
  self->lastpathid = self->targetPath;
  for(i = 0;i < MAX_PATHS;i++)
  {
    self->SkipPaths[i] = -1;
  }
  for(i = 0; i < 5; i++)
  {
    if(level.paths[self->targetPath].nextid[i] < level.numPaths &&
       level.paths[self->targetPath].nextid[i] >= 0)
    {
      if(self->OldPaths[self->targetPath].nextid[i] == level.paths[self->targetPath].nextid[i])
      {
        oldpath = qtrue;
      }
      if(oldpath == qtrue)
      {
        oldpath = qfalse;
      }
      else
      {
        possiblepaths[possiblenextpath] = level.paths[self->targetPath].nextid[i];
        possiblenextpath++;
      }
    }
  }
  if(possiblenextpath == 0)
  {	
    for(i = 0;i < level.numPaths; i++)
    {
      self->OldPaths[i].nextid[0] = -1;
      self->OldPaths[i].nextid[1] = -1;
      self->OldPaths[i].nextid[2] = -1;
      self->OldPaths[i].nextid[3] = -1;
      self->OldPaths[i].nextid[4] = -1;
    }
    self->state = FINDNEWPATH;
    return;
  }
  else
  {
    self->state = TARGETPATH;
    if(level.paths[self->targetPath].random < 0)
    {
      nextpath = 0;
    }
    else
    {
      srand( trap_Milliseconds( ) );
      randnum = (int)(( (double)rand() / ((double)(RAND_MAX)+(double)(1)) ) * possiblenextpath);
      nextpath = randnum;
      //if(nextpath == possiblenextpath)
      //{nextpath = possiblenextpath - 1;}
    }
    self->targetPath = possiblepaths[nextpath];
    for(i = 0;i < 5;i++)
    {
      if(level.paths[self->targetPath].nextid[i] == lasttarget)
      {
        self->OldPaths[self->targetPath].nextid[i] = lasttarget;
        i = 5;
      }
    }

    self->timeFoundPath = level.time;
    return;
  }
  return;
}

void pathfinding( gentity_t *self )
{
  switch(self->state)
  {
    case FINDNEWPATH: findnewpath(self); break;
    case FINDNEXTPATH: findnextpath(self); break;
    case TARGETPATH: break; //done in G_FrameThink
    case LOST: findnewpath(self);break; //LOL :(
    default: break;
  }
}

void Bot_Buy( gentity_t *self )
{
  qboolean boughtweap = qfalse;
  qboolean boughtup = qfalse;
  qboolean buybatt = qfalse;
  int weapon,upgrade;
  int maxAmmo, maxClips;
  int clientNum = self->client - level.clients;
  if(self->client->ps.stats[ STAT_PTEAM ] != PTE_HUMANS){return;}
  upgrade = UP_LIGHTARMOUR;
  if(BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ))
  {
    BG_RemoveUpgradeFromInventory( upgrade, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForUpgrade( upgrade ), qfalse );
  }
  upgrade = UP_HELMET;
  if(BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ))
  {
    BG_RemoveUpgradeFromInventory( upgrade, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForUpgrade( upgrade ), qfalse );
  }
  upgrade = UP_BATTPACK;
  if(BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ))
  {
    BG_RemoveUpgradeFromInventory( upgrade, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForUpgrade( upgrade ), qfalse );
  }
  weapon = WP_MACHINEGUN;
  if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
  {
    BG_RemoveWeaponFromInventory( weapon, self->client->ps.stats );
                //G_AddCreditToClient( self->client, (short)BG_FindPriceForWeapon( weapon ), qfalse );
  }
  weapon = WP_CHAINGUN;
  if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
  {
    BG_RemoveWeaponFromInventory( weapon, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForWeapon( weapon ), qfalse );
  }
  weapon = WP_LAS_GUN;
  if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
  {
    BG_RemoveWeaponFromInventory( weapon, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForWeapon( weapon ), qfalse );
  }
  weapon = WP_LUCIFER_CANNON;
  if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
  {
    BG_RemoveWeaponFromInventory( weapon, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForWeapon( weapon ), qfalse );
  }
  weapon = WP_FLAMER;
  if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
  {
    BG_RemoveWeaponFromInventory( weapon, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForWeapon( weapon ), qfalse );
  }
  weapon = WP_PULSE_RIFLE;
  if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
  {
    BG_RemoveWeaponFromInventory( weapon, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForWeapon( weapon ), qfalse );
  }
  weapon = WP_MASS_DRIVER;
  if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
  {
    BG_RemoveWeaponFromInventory( weapon, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForWeapon( weapon ), qfalse );
  }
  weapon = WP_SHOTGUN;
  if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
  {
    BG_RemoveWeaponFromInventory( weapon, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForWeapon( weapon ), qfalse );
  }
  weapon = WP_PAIN_SAW;
  if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
  {
    BG_RemoveWeaponFromInventory( weapon, self->client->ps.stats );
    G_AddCreditToClient( self->client, (short)BG_FindPriceForWeapon( weapon ), qfalse );
  }
  if(g_humanStage.integer == 2)
  {
    upgrade = UP_BATTLESUIT;
    if(!BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ) && 
        BG_FindPriceForUpgrade( upgrade ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
        !(BG_FindSlotsForUpgrade( upgrade ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      BG_AddUpgradeToInventory( upgrade, self->client->ps.stats );
      G_AddCreditToClient( self->client, -(short)BG_FindPriceForUpgrade( upgrade ), qfalse );
    }
  }
  upgrade = UP_LIGHTARMOUR;
  if(!BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ) && 
      BG_FindPriceForUpgrade( upgrade ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
      !(BG_FindSlotsForUpgrade( upgrade ) & self->client->ps.stats[ STAT_SLOTS ]) && 
      !BG_InventoryContainsUpgrade( UP_BATTLESUIT, self->client->ps.stats ))
  {
    BG_AddUpgradeToInventory( upgrade, self->client->ps.stats );
    G_AddCreditToClient( self->client, -(short)BG_FindPriceForUpgrade( upgrade ), qfalse );
  }
  if(g_humanStage.integer == 1)
  {
    upgrade = UP_HELMET;
    if(!BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ) && 
        BG_FindPriceForUpgrade( upgrade ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
        !(BG_FindSlotsForUpgrade( upgrade ) & self->client->ps.stats[ STAT_SLOTS ]) && 
        !BG_InventoryContainsUpgrade( UP_BATTLESUIT, self->client->ps.stats ))
    {
      BG_AddUpgradeToInventory( upgrade, self->client->ps.stats );
      G_AddCreditToClient( self->client, -(short)BG_FindPriceForUpgrade( upgrade ), qfalse );
    }
  }
  if(g_humanStage.integer == 2 && g_bot_lcannon.integer > 0)
  {
    weapon = WP_LUCIFER_CANNON;
    if( !BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) &&
         BG_FindPriceForWeapon( weapon ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
         !(BG_FindSlotsForWeapon( weapon ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      boughtweap = qtrue;
      buybatt = qtrue;
    }
  }
  if(g_humanStage.integer == 1 && boughtweap == qfalse && g_bot_flamer.integer > 0)
  {
    weapon = WP_FLAMER;
    if( !BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) &&
         BG_FindPriceForWeapon( weapon ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
         !(BG_FindSlotsForWeapon( weapon ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      boughtweap = qtrue;
    }
  }
  if(g_humanStage.integer == 1 && boughtweap == qfalse && g_bot_prifle.integer > 0)
  {
    weapon = WP_PULSE_RIFLE;
    if( !BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) &&
         BG_FindPriceForWeapon( weapon ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
         !(BG_FindSlotsForWeapon( weapon ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      boughtweap = qtrue;
      buybatt = qtrue;
    }
  }
  if(boughtweap == qfalse && g_bot_chaingun.integer > 0)
  {
    weapon = WP_CHAINGUN;
    if( !BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) &&
         BG_FindPriceForWeapon( weapon ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
         !(BG_FindSlotsForWeapon( weapon ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      boughtweap = qtrue;
    }
  }
  if(boughtweap == qfalse && g_bot_mdriver.integer > 0)
  {
    weapon = WP_MASS_DRIVER;
    if( !BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) &&
         BG_FindPriceForWeapon( weapon ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
         !(BG_FindSlotsForWeapon( weapon ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      boughtweap = qtrue;
      buybatt = qtrue;
    }
  }
  if(boughtweap == qfalse && g_bot_lasgun.integer > 0)
  {
    weapon = WP_LAS_GUN;
    if( !BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) &&
         BG_FindPriceForWeapon( weapon ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
         !(BG_FindSlotsForWeapon( weapon ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      boughtweap = qtrue;
      buybatt = qtrue;
    }
  }
  if(boughtweap == qfalse && g_bot_shotgun.integer > 0)
  {
    weapon = WP_SHOTGUN;
    if( !BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) &&
         BG_FindPriceForWeapon( weapon ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
         !(BG_FindSlotsForWeapon( weapon ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      boughtweap = qtrue;
    }
  }
  if(boughtweap == qfalse && g_bot_psaw.integer > 0)
  {
    weapon = WP_PAIN_SAW;
    if( !BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) &&
         BG_FindPriceForWeapon( weapon ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
         !(BG_FindSlotsForWeapon( weapon ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      boughtweap = qtrue;
    }
  }
  if(boughtweap == qfalse && g_bot_mgun.integer > 0)
  {
    weapon = WP_MACHINEGUN;
    if( !BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) &&
         BG_FindPriceForWeapon( weapon ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
         !(BG_FindSlotsForWeapon( weapon ) & self->client->ps.stats[ STAT_SLOTS ]))
    {
      boughtweap = qtrue;
    }
  }
  if(boughtweap == qtrue)
  {
    BG_AddWeaponToInventory( weapon, self->client->ps.stats );
    BG_FindAmmoForWeapon( weapon, &maxAmmo, &maxClips );
    if( BG_FindUsesEnergyForWeapon( weapon ) &&
        BG_InventoryContainsUpgrade( UP_BATTPACK, self->client->ps.stats ) )
      maxAmmo = (int)( (float)maxAmmo * BATTPACK_MODIFIER );
    BG_PackAmmoArray( weapon, self->client->ps.ammo, self->client->ps.powerups,
                      maxAmmo, maxClips );
    G_AddCreditToClient( self->client, -(short)BG_FindPriceForWeapon( weapon ), qfalse );
    G_ForceWeaponChange( self, weapon );
  }
  else
  {
    weapon = WP_BLASTER;
    G_ForceWeaponChange( self, weapon );
  }
  upgrade = UP_BATTPACK;
  if(!BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ) && 
      BG_FindPriceForUpgrade( upgrade ) <= (short)self->client->ps.persistant[ PERS_CREDIT ] &&
      !(BG_FindSlotsForUpgrade( upgrade ) & self->client->ps.stats[ STAT_SLOTS ]) && 
      !BG_InventoryContainsUpgrade( UP_BATTLESUIT, self->client->ps.stats ) && 
      buybatt == qtrue)
  {
    BG_AddUpgradeToInventory( upgrade, self->client->ps.stats );
    G_AddCreditToClient( self->client, -(short)BG_FindPriceForUpgrade( upgrade ), qfalse );
  }
  G_GiveClientMaxAmmo( self, qtrue );
  if(boughtup == qtrue || boughtweap == qtrue)
  {ClientUserinfoChanged( clientNum );}
  self->buytime = level.time;
  return;
}


void Bot_Evolve( gentity_t *self )
{
  vec3_t origin;
  qboolean classfound = qfalse;
  int class;
  int levels;
  int clientNum;
  if(self->client->ps.stats[ STAT_PTEAM ] != PTE_ALIENS){return;}
  clientNum = self->client - level.clients;
  class = PCL_ALIEN_LEVEL4;
  levels = BG_ClassCanEvolveFromTo( self->client->ps.stats[ STAT_PCLASS ],
                                    class,
                                    (short)self->client->ps.persistant[ PERS_CREDIT ], 0 );
  if(BG_ClassIsAllowed( class ) && 
     BG_FindStagesForClass( class, g_alienStage.integer ) && 
     level.overmindPresent &&
     G_RoomForClassChange( self, class, origin ) && 
     !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) && 
     !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING ) &&
     levels >= 0)
  {
    classfound = qtrue;
  }
  if(classfound == qfalse)
  {
    class = PCL_ALIEN_LEVEL3_UPG;
    levels = BG_ClassCanEvolveFromTo( self->client->ps.stats[ STAT_PCLASS ],
                                      class,
                                      (short)self->client->ps.persistant[ PERS_CREDIT ], 0 );
    if(BG_ClassIsAllowed( class ) && 
       BG_FindStagesForClass( class, g_alienStage.integer ) && 
       level.overmindPresent &&
       G_RoomForClassChange( self, class, origin ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING ) &&
       levels >= 0)
    {
      classfound = qtrue;
    }
  }
  if(classfound == qfalse)
  {
    class = PCL_ALIEN_LEVEL3;
    levels = BG_ClassCanEvolveFromTo( self->client->ps.stats[ STAT_PCLASS ],
                                      class,
                                      (short)self->client->ps.persistant[ PERS_CREDIT ], 0 );
    if(BG_ClassIsAllowed( class ) && 
       BG_FindStagesForClass( class, g_alienStage.integer ) && 
       level.overmindPresent &&
       G_RoomForClassChange( self, class, origin ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING ) &&
       levels >= 0)
    {
      classfound = qtrue;
    }
  }
  if(classfound == qfalse)
  {
    class = PCL_ALIEN_LEVEL2_UPG;
    levels = BG_ClassCanEvolveFromTo( self->client->ps.stats[ STAT_PCLASS ],
                                      class,
                                      (short)self->client->ps.persistant[ PERS_CREDIT ], 0 );
    if(BG_ClassIsAllowed( class ) && 
       BG_FindStagesForClass( class, g_alienStage.integer ) && 
       level.overmindPresent &&
       G_RoomForClassChange( self, class, origin ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING ) &&
       levels >= 0)
    {
      classfound = qtrue;
    }
  }
  if(classfound == qfalse)
  {
    class = PCL_ALIEN_LEVEL2;
    levels = BG_ClassCanEvolveFromTo( self->client->ps.stats[ STAT_PCLASS ],
                                      class,
                                      (short)self->client->ps.persistant[ PERS_CREDIT ], 0 );
    if(BG_ClassIsAllowed( class ) && 
       BG_FindStagesForClass( class, g_alienStage.integer ) && 
       level.overmindPresent &&
       G_RoomForClassChange( self, class, origin ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING ) &&
       levels >= 0)
    {
      classfound = qtrue;
    }
  }
  if(classfound == qfalse)
  {
    class = PCL_ALIEN_LEVEL1_UPG;
    levels = BG_ClassCanEvolveFromTo( self->client->ps.stats[ STAT_PCLASS ],
                                      class,
                                      (short)self->client->ps.persistant[ PERS_CREDIT ], 0 );
    if(BG_ClassIsAllowed( class ) && 
       BG_FindStagesForClass( class, g_alienStage.integer ) && 
       level.overmindPresent &&
       G_RoomForClassChange( self, class, origin ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING ) &&
       levels >= 0)
    {
      classfound = qtrue;
    }
  }
  if(classfound == qfalse)
  {
    class = PCL_ALIEN_LEVEL1;
    levels = BG_ClassCanEvolveFromTo( self->client->ps.stats[ STAT_PCLASS ],
                                      class,
                                      (short)self->client->ps.persistant[ PERS_CREDIT ], 0 );
    if(BG_ClassIsAllowed( class ) && 
       BG_FindStagesForClass( class, g_alienStage.integer ) && 
       level.overmindPresent &&
       G_RoomForClassChange( self, class, origin ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) && 
       !( self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING ) &&
       levels >= 0)
    {
      classfound = qtrue;
    }
  }
  if(classfound == qtrue)
  {
    self->client->pers.evolveHealthFraction = (float)self->client->ps.stats[ STAT_HEALTH ] /
        (float)BG_FindHealthForClass( self->client->ps.stats[ STAT_PCLASS ] );
    if( self->client->pers.evolveHealthFraction < 0.0f )
      self->client->pers.evolveHealthFraction = 0.0f;
    else if( self->client->pers.evolveHealthFraction > 1.0f )
      self->client->pers.evolveHealthFraction = 1.0f;
    G_AddCreditToClient( self->client, -(short)levels, qtrue );
    self->client->pers.classSelection = class;
    ClientUserinfoChanged( clientNum );
    VectorCopy( origin, self->s.pos.trBase );
    ClientSpawn( self, self, self->s.pos.trBase, self->s.apos.trBase );	
    self->evolvetime = level.time;
    return;
  }
  self->evolvetime = level.time;
  return;
}

void G_BotThink( gentity_t *self )
{
  int distance = 0;
  int clicksToStopChase = 30; //5 seconds
  int tooCloseDistance = 100; // about 1/3 of turret range
  int forwardMove = 127; // max speed
  int tempEntityIndex = -1;
  trace_t   tr;
  vec3_t    end;
  vec3_t    mins, maxs;
  vec3_t  forward, right, up;
  vec3_t  muzzle;
  gentity_t *traceEnt;
  qboolean followFriend = qfalse;
  if( !( self->r.svFlags & SVF_BOT ) )
  {
    return;
  }
  self->isblocked = qfalse;
  if(self->state == TARGETPATH && !self->botEnemy && !self->botFriend)
  {
    //AngleVectors( self->client->ps.viewangles, forward, right, up );
    //CalcMuzzlePoint( self, forward, right, up, muzzle );
    VectorSet( mins, -20, -20, -20 );
    VectorSet( maxs, 20, 20, 20 );
    AngleVectors( self->client->ps.viewangles, forward, right, up );
    CalcMuzzlePoint( self, forward, right, up, muzzle );
    VectorMA( muzzle, 20, forward, end );
    trap_Trace( &tr, muzzle, mins, maxs, end, self->s.number, MASK_SHOT );
    traceEnt = &g_entities[ tr.entityNum ];
    if(traceEnt->health > 0)
    {self->isblocked = qtrue;}
    else
    {self->isblocked = qfalse;}
  }
  if(self->botEnemy || self->botFriend)
  {
    //self->client->pers.cmd.buttons = 0;
    self->client->pers.cmd.forwardmove = 0;
    self->client->pers.cmd.upmove = 0;
    self->client->pers.cmd.rightmove = 0;
    self->timeFoundPath = level.time;
  }
  // reset botEnemy if enemy is dead
  if(self->botEnemy->health <= 0) {
    self->botEnemy = NULL;
  }
  else
  {
    if(botTargetInRange( self, self->botEnemy ) == qfalse)
    {
      self->botEnemy = NULL;
    }
  }
  // if friend dies, reset status to regular
  if(self->botFriend->health <= 0) {
    self->botCommand = BOT_REGULAR;
    self->botFriend = NULL;
  }
  //Use blaster if weapon out of ammo
  if(self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS && 
     self->noammo == qtrue &&
     self->client->ps.weapon != WP_BLASTER)
  {G_ForceWeaponChange( self, WP_BLASTER );}

  //Use medkit if low hp
  if(self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS && 
     BG_InventoryContainsUpgrade( UP_MEDKIT, self->client->ps.stats ) &&
     self->health <= 40)
  {BG_ActivateUpgrade( UP_MEDKIT, self->client->ps.stats );}

  //Buy stuff from arm if within range
  if(G_BuildableRange( self->client->ps.origin, 100, BA_H_ARMOURY ) && 
     self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS &&
     level.time - self->buytime > 5000)
  {
    Bot_Buy(self);
  }
  if(self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS &&
     level.time - self->evolvetime > 10000 && 
     g_bot_evolve.integer > 0)
  {Bot_Evolve(self);}
  if(self->client->ps.weapon == WP_MACHINEGUN && g_bot_mgun.integer <= 0)
  {
    G_ForceWeaponChange( self, WP_BLASTER );
  }
  if(self->client->ps.weapon == WP_SHOTGUN && g_bot_shotgun.integer <= 0)
  {
    G_ForceWeaponChange( self, WP_BLASTER );
  }
  if(self->client->ps.weapon == WP_PAIN_SAW && g_bot_psaw.integer <= 0)
  {
    G_ForceWeaponChange( self, WP_BLASTER );
  }
  if(self->client->ps.weapon == WP_LAS_GUN && g_bot_lasgun.integer <= 0)
  {
    G_ForceWeaponChange( self, WP_BLASTER );
  }
  if(self->client->ps.weapon == WP_MASS_DRIVER && g_bot_mdriver.integer <= 0)
  {
    G_ForceWeaponChange( self, WP_BLASTER );
  }
  if(self->client->ps.weapon == WP_PULSE_RIFLE && g_bot_prifle.integer <= 0)
  {
    G_ForceWeaponChange( self, WP_BLASTER );
  }
  if(self->client->ps.weapon == WP_FLAMER && g_bot_flamer.integer <= 0)
  {
    G_ForceWeaponChange( self, WP_BLASTER );
  }
  if(self->client->ps.weapon == WP_LUCIFER_CANNON && g_bot_lcannon.integer <= 0)
  {
    G_ForceWeaponChange( self, WP_BLASTER );
  }
  if(self->client->ps.weapon == WP_CHAINGUN && g_bot_chaingun.integer <= 0)
  {
    G_ForceWeaponChange( self, WP_BLASTER );
  }
   // what mode are we in?
  switch(self->botCommand) {
    case BOT_REGULAR:
                        // if there is enemy around, rush them and attack.
      if(self->botEnemy) {
                                // we already have an enemy. See if still in LOS.
        if(!botTargetInRange(self,self->botEnemy)) {
                                        // if it's been over clicksToStopChase clicks since we last seen him in LOS then do nothing, else follow him!
          if(self->botEnemyLastSeen > clicksToStopChase) {
                                                // forget him!
            self->botEnemy = NULL;
            self->botEnemyLastSeen = 0;
          } else {
                                                //chase him
            self->botEnemyLastSeen++;
          }
        } else {
                                        // we see him!
          self->botEnemyLastSeen = 0;
        }
      }

      if(!self->botEnemy) {
                                // try to find closest enemy
        if(level.time - self->searchtime > 750 * self->botSkillLevel)
        {
          self->searchtime = level.time;
          tempEntityIndex = botFindClosestEnemy(self, qfalse);
          if(tempEntityIndex >= 0)
            self->botEnemy = &g_entities[tempEntityIndex];
        }
      }

      if(!self->botEnemy) {
        pathfinding(self); //Roam the map!!!
      } else {
        self->timeFoundPath = level.time;
        self->state = FINDNEWPATH;
                                // enemy!
        distance = botGetDistanceBetweenPlayer(self, self->botEnemy);
        botAimAtTarget(self, self->botEnemy);

        // enable wallwalk
        if( BG_ClassHasAbility( self->client->ps.stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) )
        {
          self->client->pers.cmd.upmove = -1;
        }
        else if( self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS && level.time - self->jumptime > 3000 && 
                 self->client->ps.stats[ STAT_PCLASS ] != PCL_ALIEN_LEVEL3_UPG && 
                 self->client->ps.stats[ STAT_PCLASS ] != PCL_ALIEN_LEVEL3)
        {
                                        //self->client->ps.velocity[2] = BG_FindJumpMagnitudeForClass( self->client->ps.stats[ STAT_PCLASS ] );
          self->client->pers.cmd.upmove = 20;
          if(level.time - self->jumptime > 4000)
            self->jumptime = level.time;
        }

        botShootIfTargetInRange(self,self->botEnemy);

        if(self->botEnemy->client || self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS)
        {
          if(self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS && 
             g_human_strafe.integer > 0)
          {
            self->client->pers.cmd.rightmove = -100;
            if(self->client->time1000 >= 500)
              self->client->pers.cmd.rightmove = 100;
          }
          //self->client->pers.cmd.forwardmove = forwardMove;
          if(self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS && 
             Distance( self->s.pos.trBase, self->botEnemy->s.pos.trBase ) < 300 && 
             self->client->ps.weapon != WP_PAIN_SAW && 
             self->client->ps.weapon != WP_FLAMER)
          {
            self->client->pers.cmd.forwardmove = -100;
          }
          else
          {
            self->client->pers.cmd.forwardmove = forwardMove;
            self->client->pers.cmd.rightmove = -100;
            if(self->client->time1000 >= 500)
              self->client->pers.cmd.rightmove = 100;
          }
        }
        else
        {
          if(self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS)//Human target buildable
          {
            if(self->client->ps.weapon == WP_PAIN_SAW)
            {
              self->client->pers.cmd.forwardmove = forwardMove;
            }
            else if(self->client->ps.weapon == WP_FLAMER)
            {
              self->client->pers.cmd.forwardmove = forwardMove;
            }
            else
            {
              self->client->pers.cmd.upmove = -1;
            }
          }
        }
      }
      break;
    case BOT_IDLE:
       // just stand there and look pretty.
      break;
    case BOT_ATTACK:
      // .. not sure ..
      break;
    case BOT_STAND_GROUND:
      // stand ground but attack enemies if you can reach.
      if(self->botEnemy) {
        // we already have an enemy. See if still in LOS.
        if(!botTargetInRange(self,self->botEnemy)) {
          //we are not in LOS
          self->botEnemy = NULL;
        }
      }
      if(!self->botEnemy) {
        // try to find closest enemy
        tempEntityIndex = botFindClosestEnemy(self, qfalse);
        if(tempEntityIndex >= 0)
          self->botEnemy = &g_entities[tempEntityIndex];
      }
      if(!self->botEnemy) {
        // no enemy
      } else {
        // enemy!
        distance = botGetDistanceBetweenPlayer(self, self->botEnemy);
        //botAimAtTarget(self, self->botEnemy);

        // enable wallwalk
        if( BG_ClassHasAbility( self->client->ps.stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) ) {
          self->client->pers.cmd.upmove = -1;
        }
        botShootIfTargetInRange(self,self->botEnemy);
      }
      break;
    case BOT_DEFENSIVE:
      // if there is an enemy around, rush them but not too far from where you are standing when given this command
      break;
    case BOT_FOLLOW_FRIEND_PROTECT:
      // run towards friend, attack enemy
      break;
    case BOT_FOLLOW_FRIEND_ATTACK:
      // run with friend until enemy spotted, then rush enemy
      if(self->botEnemy) {
        // we already have an enemy. See if still in LOS.
        if(!botTargetInRange(self,self->botEnemy)) {
          // if it's been over clicksToStopChase clicks since we last seen him in LOS then do nothing, else follow him!
          if(self->botEnemyLastSeen > clicksToStopChase) {
            // forget him!
            self->botEnemy = NULL;
            self->botEnemyLastSeen = 0;
          } else {
            //chase him
            self->botEnemyLastSeen++;
          }
        } else {
          // we see him!
          self->botEnemyLastSeen = 0;
        }

        //if we are chasing enemy, reset counter for friend LOS .. if its true
        if(self->botEnemy) {
          if(botTargetInRange(self,self->botFriend)) {
            self->botFriendLastSeen = 0;
          } else {
            self->botFriendLastSeen++;
          }
        }
      }

      if(!self->botEnemy) {
        // try to find closest enemy
        tempEntityIndex = botFindClosestEnemy(self, qfalse);
        if(tempEntityIndex >= 0)
          self->botEnemy = &g_entities[tempEntityIndex];
      }

      if(!self->botEnemy) {
        // no enemy
        if(self->botFriend) {
          // see if our friend is in LOS
          if(botTargetInRange(self,self->botFriend)) {
            // go to him!
            followFriend = qtrue;
            self->botFriendLastSeen = 0;
          } else {
            // if it's been over clicksToStopChase clicks since we last seen him in LOS then do nothing, else follow him!
            if(self->botFriendLastSeen > clicksToStopChase) {
              // forget him!
              followFriend = qfalse;
            } else {
              self->botFriendLastSeen++;
              followFriend = qtrue;
            }
          }

          if(followFriend) {
            distance = botGetDistanceBetweenPlayer(self, self->botFriend);
            //botAimAtTarget(self, self->botFriend);

            // enable wallwalk
            if( BG_ClassHasAbility( self->client->ps.stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) ) {
              self->client->pers.cmd.upmove = -1;
            }

            //botShootIfTargetInRange(self,self->botEnemy);
            if(distance>tooCloseDistance) {
              self->client->pers.cmd.forwardmove = forwardMove;
              self->client->pers.cmd.rightmove = -100;
              if(self->client->time1000 >= 500)
                self->client->pers.cmd.rightmove = 100;
            }
          }
        }
      } else {
         // enemy!
        distance = botGetDistanceBetweenPlayer(self, self->botEnemy);
        //botAimAtTarget(self, self->botEnemy);

        // enable wallwalk
        if( BG_ClassHasAbility( self->client->ps.stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) ) {
          self->client->pers.cmd.upmove = -1;
        }

        botShootIfTargetInRange(self,self->botEnemy);
        self->client->pers.cmd.forwardmove = forwardMove;
        self->client->pers.cmd.rightmove = -100;
        if(self->client->time1000 >= 500)
          self->client->pers.cmd.rightmove = 100;
      }
      break;
    case BOT_FOLLOW_FRIEND_IDLE:
      // run with friend and stick with him no matter what. no attack mode.
      if(self->botFriend) {
        // see if our friend is in LOS
        if(botTargetInRange(self,self->botFriend)) {
                                        // go to him!
          followFriend = qtrue;
          self->botFriendLastSeen = 0;
        } else {
          // if it's been over clicksToStopChase clicks since we last seen him in LOS then do nothing, else follow him!
          if(self->botFriendLastSeen > clicksToStopChase) {
            // forget him!
            followFriend = qfalse;
          } else {
            //chase him
            self->botFriendLastSeen++;
            followFriend = qtrue;
          }
        }

        if(followFriend) {
          distance = botGetDistanceBetweenPlayer(self, self->botFriend);
          //botAimAtTarget(self, self->botFriend);

          // enable wallwalk
          if( BG_ClassHasAbility( self->client->ps.stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) ) {
            self->client->pers.cmd.upmove = -1;
          }

          //botShootIfTargetInRange(self,self->botFriend);
          if(distance>tooCloseDistance) {
            self->client->pers.cmd.forwardmove = forwardMove;
            self->client->pers.cmd.rightmove = -100;
            if(self->client->time1000 >= 500)
              self->client->pers.cmd.rightmove = 100;
          }
        }
      }
      break;
    case BOT_TEAM_KILLER:
      // attack enemies, then teammates!
      if(self->botEnemy) {
        // we already have an enemy. See if still in LOS.
        if(!botTargetInRange(self,self->botEnemy)) {
          // if it's been over clicksToStopChase clicks since we last seen him in LOS then do nothing, else follow him!
          if(self->botEnemyLastSeen > clicksToStopChase) {
            // forget him!
            self->botEnemy = NULL;
            self->botEnemyLastSeen = 0;
          } else {
            //chase him
            self->botEnemyLastSeen++;
          }
        } else {
          // we see him!
          self->botEnemyLastSeen = 0;
        }
      }

      if(!self->botEnemy) {
        // try to find closest enemy
        tempEntityIndex = botFindClosestEnemy(self, qtrue);
        if(tempEntityIndex >= 0)
          self->botEnemy = &g_entities[tempEntityIndex];
      }

      if(!self->botEnemy) {
        // no enemy, we're all alone :(
      } else {
        // enemy!
        distance = botGetDistanceBetweenPlayer(self, self->botEnemy);
        //botAimAtTarget(self, self->botEnemy);

        // enable wallwalk
        if( BG_ClassHasAbility( self->client->ps.stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) ) {
          self->client->pers.cmd.upmove = -1;
        }

        botShootIfTargetInRange(self,self->botEnemy);
        self->client->pers.cmd.forwardmove = forwardMove;
        self->client->pers.cmd.rightmove = -100;
        if(self->client->time1000 >= 500)
          self->client->pers.cmd.rightmove = 100;
      }
      break;
    default:
      // dunno.
      break;
  }
}

void G_BotSpectatorThink( gentity_t *self ) {
  if( self->client->ps.pm_flags & PMF_QUEUED) {
    //we're queued to spawn, all good
    return;
  }

  if( self->client->sess.sessionTeam == TEAM_SPECTATOR ) {
    int teamnum = self->client->pers.teamSelection;
    int clientNum = self->client->ps.clientNum;

    if( teamnum == PTE_HUMANS ) {
      self->client->pers.classSelection = PCL_HUMAN;
      self->client->ps.stats[ STAT_PCLASS ] = PCL_HUMAN;
      self->client->pers.humanItemSelection = WP_MACHINEGUN;
      G_PushSpawnQueue( &level.humanSpawnQueue, clientNum );
    } else if( teamnum == PTE_ALIENS) {
      self->client->pers.classSelection = PCL_ALIEN_LEVEL0;
      self->client->ps.stats[ STAT_PCLASS ] = PCL_ALIEN_LEVEL0;
      G_PushSpawnQueue( &level.alienSpawnQueue, clientNum );
    }
  }
}


qboolean botAimAtTarget( gentity_t *self, gentity_t *target ) {
  int Ax,Ay,Az,Bx,By,Bz = 0;
  vec3_t dirToTarget, angleToTarget;
  vec3_t top = { 0, 0, 0};
  vec3_t  forward, right, up;
  vec3_t  muzzle;
  AngleVectors( self->client->ps.viewangles, forward, right, up );
  CalcMuzzlePoint( self, forward, right, up, muzzle );
  if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL3 || 
     self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL3_UPG)
  {
    Ax = target->s.pos.trBase[0];
    Ay = target->s.pos.trBase[1];
    Az = target->s.pos.trBase[2];
    Bx = self->s.pos.trBase[0];
    By = self->s.pos.trBase[1];
    Bz = self->s.pos.trBase[2];
    if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL3)
    {top[2] = (int)(sqrt((Ax - Bx)*(Ax - Bx) + (Ay - By)*(Ay - By) + (Az - Bz)*(Az - Bz)) / 3);}
    else
    {top[2] = (int)(sqrt((Ax - Bx)*(Ax - Bx) + (Ay - By)*(Ay - By) + (Az - Bz)*(Az - Bz)) / 5);}
  }
  VectorAdd( target->s.pos.trBase, top, top);
  VectorSubtract( top, muzzle, dirToTarget );
  VectorNormalize( dirToTarget );
  vectoangles( dirToTarget, angleToTarget );
  self->client->ps.delta_angles[ 0 ] = ANGLE2SHORT( angleToTarget[ 0 ] );
  self->client->ps.delta_angles[ 1 ] = ANGLE2SHORT( angleToTarget[ 1 ] );
  self->client->ps.delta_angles[ 2 ] = ANGLE2SHORT( angleToTarget[ 2 ] );
  return qtrue;
}

qboolean botTargetInRange( gentity_t *self, gentity_t *target ) {
  trace_t   trace;
  gentity_t *traceEnt;
  vec3_t  forward, right, up;
  vec3_t  muzzle;
  AngleVectors( self->client->ps.viewangles, forward, right, up );
  CalcMuzzlePoint( self, forward, right, up, muzzle );

  if( !self || !target )
    return qfalse;

  if( !self->client || ( !target->client && g_bot_attackbuildables.integer == 0 ) )
    return qfalse;

  if( target->client->ps.stats[ STAT_STATE ] & SS_HOVELING )
    return qfalse;

  if( target->health <= 0 )
    return qfalse;

  if( self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS &&
      self->client->ps.stats[ STAT_PCLASS ] != PCL_ALIEN_LEVEL3_UPG &&
      self->client->ps.stats[ STAT_PCLASS ] != PCL_ALIEN_LEVEL3 &&
      target->s.pos.trBase[2] - self->s.pos.trBase[2] > 150)
    return qfalse;
  if(self->client->ps.weapon == WP_PAIN_SAW && 
     target->s.pos.trBase[2] - self->s.pos.trBase[2] > 150)
  {
    return qfalse;
  }
  else if(self->client->ps.weapon == WP_FLAMER && 
          target->s.pos.trBase[2] - self->s.pos.trBase[2] > 200)
  {
    return qfalse;
  }
  if(self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS)
  {
    if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL4 && 
       Distance( self->s.pos.trBase, target->s.pos.trBase ) > g_level4_range.integer)
    {return qfalse;}
    else if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL3_UPG && 
            Distance( self->s.pos.trBase, target->s.pos.trBase ) > g_level3UPG_range.integer)
    {return qfalse;}
    else if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL3 && 
            Distance( self->s.pos.trBase, target->s.pos.trBase ) > g_level3_range.integer)
    {return qfalse;}
    else if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL2_UPG && 
            Distance( self->s.pos.trBase, target->s.pos.trBase ) > g_level2UPG_range.integer)
    {return qfalse;}
    else if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL2 && 
            Distance( self->s.pos.trBase, target->s.pos.trBase ) > g_level2_range.integer)
    {return qfalse;}
    else if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL1_UPG && 
            Distance( self->s.pos.trBase, target->s.pos.trBase ) > g_level1UPG_range.integer)
    {return qfalse;}
    else if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL1 && 
            Distance( self->s.pos.trBase, target->s.pos.trBase ) > g_level1_range.integer)
    {return qfalse;}
    else if(self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL0 && 
            Distance( self->s.pos.trBase, target->s.pos.trBase ) > g_level0_range.integer)
    {return qfalse;}
    else if((self->client->ps.stats[ STAT_PCLASS ] == PCL_HUMAN || 
             self->client->ps.stats[ STAT_PCLASS ] == PCL_HUMAN_BSUIT ) && 
             Distance( self->s.pos.trBase, target->s.pos.trBase ) > g_human_range.integer)
    {return qfalse;}
  }
  else
  {
    if(Distance( self->s.pos.trBase, target->s.pos.trBase ) > 3000)
    {return qfalse;}
  }
  //BG_FindViewheightForClass(  self->client->ps.stats[ STAT_PCLASS ], &vh, NULL );
  //top[2]=vh;
  //VectorAdd( self->s.pos.trBase, top, top);
  //draw line between us and the target and see what we hit
  //trap_Trace( &trace, self->s.pos.trBase, NULL, NULL, target->s.pos.trBase, self->s.number, MASK_SHOT );

  trap_Trace( &trace, muzzle, NULL, NULL, target->s.pos.trBase, self->s.number, MASK_SHOT );
  traceEnt = &g_entities[ trace.entityNum ];
  //if( trace.fraction < 1.0 )
  //{return qfalse;}
  // check that we hit a human and not an object
  //if( !traceEnt->client )
  //	return qfalse;

  //check our target is in LOS
  if(!(traceEnt == target))
    return qfalse;

  return qtrue;
}

int botFindClosestEnemy( gentity_t *self, qboolean includeTeam ) {
  // return enemy entity index, or -1
  int vectorRange = MGTURRET_RANGE * 3;
  int i;
  int total_entities;
  int entityList[ MAX_GENTITIES ];
  vec3_t    range;
  vec3_t    mins, maxs;
  gentity_t *target;

  VectorSet( range, vectorRange, vectorRange, vectorRange );
  VectorAdd( self->client->ps.origin, range, maxs );
  VectorSubtract( self->client->ps.origin, range, mins );

  total_entities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

  // check list for enemies
  for( i = 0; i < total_entities; i++ ) {
    target = &g_entities[ entityList[ i ] ];

    if ( g_bot_attackbuildables.integer == 0 )
    {
      if (target->client && self != target && target->client->ps.stats[ STAT_PTEAM ] != self->client->ps.stats[ STAT_PTEAM ])
      {
        // aliens ignore if it's in LOS because they have radar
        //if(self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS) {
        //	return entityList[ i ];
        //} else {
        if( botTargetInRange( self, target ) ) {
          return entityList[ i ];
        }
        //}
      }
    }
    else if ( g_bot_attackbuildables.integer == 1 )
    {
      if ((target->client && self != target && target->client->ps.stats[ STAT_PTEAM ] != self->client->ps.stats[ STAT_PTEAM ])
           || (!target->client && self != target && self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS && self->client->ps.stats[ STAT_PCLASS ] != PCL_ALIEN_LEVEL0 &&
           (target->s.modelindex == BA_H_SPAWN
           || target->s.modelindex == BA_H_MGTURRET
           || target->s.modelindex == BA_H_TESLAGEN
           || target->s.modelindex == BA_H_ARMOURY
           || target->s.modelindex == BA_H_DCC
           || target->s.modelindex == BA_H_MEDISTAT
           || target->s.modelindex == BA_H_REACTOR
           || target->s.modelindex == BA_H_REPEATER)) ||
           (!target->client && self != target && self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS && (
             target->s.modelindex == BA_A_SPAWN ||
           target->s.modelindex == BA_A_BARRICADE ||
           target->s.modelindex == BA_A_BOOSTER ||
           target->s.modelindex == BA_A_ACIDTUBE ||
           target->s.modelindex == BA_A_HIVE ||
           target->s.modelindex == BA_A_TRAPPER ||
           target->s.modelindex == BA_A_OVERMIND ||
           target->s.modelindex == BA_A_HOVEL)) || (!target->client && self != target && self->client->ps.stats[ STAT_PCLASS ] == PCL_ALIEN_LEVEL0 && (target->s.modelindex == BA_H_MGTURRET)))
      {
        // aliens ignore if it's in LOS because they have radar
        //if(self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS) {
        //	return entityList[ i ];
        //} else {
        if( botTargetInRange( self, target ) ) {
          return entityList[ i ];
        }
        //}
      }
    }
  }

  if(includeTeam) {
    // check list for enemies in team
    for( i = 0; i < total_entities; i++ ) {
      target = &g_entities[ entityList[ i ] ];

      if( target->client && self !=target && target->client->ps.stats[ STAT_PTEAM ] == self->client->ps.stats[ STAT_PTEAM ] ) {
        // aliens ignore if it's in LOS because they have radar
        //if(self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS) {
        //	return entityList[ i ];
        //} else {
        if( botTargetInRange( self, target ) ) {
          return entityList[ i ];
        }
        //}
      }
    }
  }

  return -1;
}

// really an int? what if it's too long?
int botGetDistanceBetweenPlayer( gentity_t *self, gentity_t *player ) {
  return Distance( self->s.pos.trBase, player->s.pos.trBase );
}

qboolean botShootIfTargetInRange( gentity_t *self, gentity_t *target )
{
  if(botTargetInRange(self,target))
  {
    int nahoda = 0;
    srand( trap_Milliseconds( ) );
    //nahoda = (rand() % 20);
    nahoda = (int)(( (double)rand() / ((double)(RAND_MAX)+(double)(1)) ) * 20);
    self->client->pers.cmd.buttons = 0;
    if (self->client->pers.classSelection == PCL_ALIEN_BUILDER0_UPG)//adv granger
    {
      if (nahoda > 10)
        self->client->pers.cmd.buttons |= BUTTON_ATTACK2;
      else
        self->client->pers.cmd.buttons |= BUTTON_USE_HOLDABLE;
    }
    else if (self->client->pers.classSelection == PCL_ALIEN_LEVEL1_UPG)//adv basilisk
    {
      if (nahoda > 10)
        self->client->pers.cmd.buttons |= BUTTON_ATTACK2;
      else
        self->client->pers.cmd.buttons |= BUTTON_ATTACK;
    }
    else if (self->client->pers.classSelection == PCL_ALIEN_LEVEL2_UPG)//adv marauder
    {
      if (nahoda < 8)
        self->client->pers.cmd.buttons |= BUTTON_ATTACK2;
      else
        self->client->pers.cmd.buttons |= BUTTON_ATTACK;
    }
    else if (self->client->pers.classSelection == PCL_ALIEN_LEVEL3)//dragon
    {
      if(Distance( self->s.pos.trBase, target->s.pos.trBase ) > 150 && 
         self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_SPEED)
        self->client->pers.cmd.buttons |= BUTTON_ATTACK2;
      else
        self->client->pers.cmd.buttons |= BUTTON_ATTACK;
    }
    else if (self->client->pers.classSelection == PCL_ALIEN_LEVEL3_UPG)//adv dragon
    {
      if(self->client->ps.ammo[WP_ALEVEL3_UPG] > 0 && 
         Distance( self->s.pos.trBase, target->s.pos.trBase ) > 150 )
        self->client->pers.cmd.buttons |= BUTTON_USE_HOLDABLE;
      else
      {	
        if(Distance( self->s.pos.trBase, target->s.pos.trBase ) > 150 && 
           self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_UPG_SPEED)
          self->client->pers.cmd.buttons |= BUTTON_ATTACK2;
        else
          self->client->pers.cmd.buttons |= BUTTON_ATTACK;
      }
    }
    else//others
    {
      if(self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS)//Human target buildable
      {
        if(self->client->ps.weapon == WP_FLAMER)
        {
          if(Distance( self->s.pos.trBase, target->s.pos.trBase ) < 200)
            self->client->pers.cmd.buttons |= BUTTON_ATTACK;
        }
        else if(self->client->ps.weapon == WP_LUCIFER_CANNON)
        {
          self->client->pers.cmd.buttons |= BUTTON_ATTACK2;
        }
        else
        {
          self->client->pers.cmd.buttons |= BUTTON_ATTACK;
        }
      }
      else
      {
        self->client->pers.cmd.buttons |= BUTTON_ATTACK;
      }
    }
    //if (nahoda == 15 || nahoda == 16)
    //	self->client->pers.cmd.buttons |= BUTTON_GESTURE;
    //if (nahoda > 11 && nahoda < 15)
    //	self->client->pers.cmd.upmove = 20;
  }
  return qfalse;
}
