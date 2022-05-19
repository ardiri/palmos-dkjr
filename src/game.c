/*
 * @(#)game.c
 *
 * Copyright 1999-2000, Aaron Ardiri (mailto:aaron@ardiri.com)
 * All rights reserved.
 *
 * This file was generated as part of the "dkjr" program developed for 
 * the Palm Computing Platform designed by Palm: 
 *
 *   http://www.palm.com/ 
 *
 * The contents of this file is confidential and proprietrary in nature 
 * ("Confidential Information"). Redistribution or modification without 
 * prior consent of the original author is prohibited. 
 */

#include "palm.h"

// interface

static void GameDrawRestore(PreferencesType *)                    __GAME__;
static void GameAdjustLevel(PreferencesType *)                    __GAME__;
static void GameIncrementScore(PreferencesType *)                 __GAME__;
static void GameMovePlayer(PreferencesType *)                     __GAME__;
static void GameMoveChops(PreferencesType *)                      __GAME__;
static void GameRemoveChop(PreferencesType *, UInt16)             __GAME__;
static void GameMoveBirds(PreferencesType *)                      __GAME__;
static void GameRemoveBird(PreferencesType *, UInt16)             __GAME__;

// global variable structure
typedef struct
{
  WinHandle winDigits;                      // scoring digits bitmaps
  WinHandle winMisses;                      // the lives notification bitmaps

  WinHandle winCages;                       // the cage bitmaps
  WinHandle winCageBackup;                  // the area behind the cage
  Boolean   cageChanged;                    // do we need to repaint the cage?
  Boolean   cageBackupAvailable;            // is there a backup available?

  WinHandle winKeys;                        // the key bitmaps
  Boolean   keyChanged;                     // do we need to repaint the key?
  Boolean   keyOnScreen;                    // is key bitmap on screen?
  UInt16    keyOldPosition;                 // the *old* position of the key 

  WinHandle winDrops;                       // the drop bitmaps
  Boolean   dropChanged;                    // do we need to repaint the drop?
  Boolean   dropOnScreen;                   // is drop bitmap on screen?
  UInt16    dropOldPosition;                // the *old* position of the drop 

  WinHandle winBirds;                       // the bird bitmaps
  Boolean   birdChanged[MAX_BIRDS];         // do we need to repaint the bird?
  Boolean   birdOnScreen[MAX_BIRDS];        // is bird bitmap on screen?
  UInt16    birdOnScreenPosition[MAX_BIRDS];// the *old* position of the bird

  WinHandle winChops;                       // the chop bitmaps
  Boolean   chopChanged[MAX_CHOPS];         // do we need to repaint the chop?
  Boolean   chopOnScreen[MAX_CHOPS];        // is chop bitmap on screen?
  UInt16    chopOnScreenPosition[MAX_CHOPS];// the *old* position of the chop

  WinHandle winKongs;                       // the kong bitmaps
  Boolean   kongChanged;                    // do we need to repaint DK?
  Boolean   kongOnScreen;                   // is DK bitmap on screen?
  UInt16    kongOldPosition;                // the *old* position of DK 

  UInt8     gameType;                       // the type of game active
  Boolean   playerDied;                     // has the player died?
  UInt8     moveDelayCount;                 // the delay between moves
  UInt8     moveLast;                       // the last move performed
  UInt8     moveNext;                       // the next desired move

  struct {

    Boolean    gamePadPresent;              // is the gamepad driver present
    UInt16     gamePadLibRef;               // library reference for gamepad

  } hardware;

} GameGlobals;

/**
 * Initialize the Game.
 * 
 * @return true if game is initialized, false otherwise
 */  
Boolean   
GameInitialize()
{
  GameGlobals *gbls;
  Err         err;
  Boolean     result;

  // create the globals object, and register it
  gbls = (GameGlobals *)MemPtrNew(sizeof(GameGlobals));
  MemSet(gbls, sizeof(GameGlobals), 0);
  FtrSet(appCreator, ftrGameGlobals, (UInt32)gbls);

  // load the gamepad driver if available
  {
    Err err;

    // attempt to load the library
    err = SysLibFind(GPD_LIB_NAME,&gbls->hardware.gamePadLibRef);
    if (err == sysErrLibNotFound)
      err = SysLibLoad('libr',GPD_LIB_CREATOR,&gbls->hardware.gamePadLibRef);

    // lets determine if it is available
    gbls->hardware.gamePadPresent = (err == errNone);

    // open the library if available
    if (gbls->hardware.gamePadPresent)
      GPDOpen(gbls->hardware.gamePadLibRef);
  }

  // initialize our "bitmap" windows
  err = errNone;
  {
    UInt16 i;
    Err    e;

    gbls->winDigits = 
      WinCreateOffscreenWindow(70, 12, screenFormat, &e); err |= e;

    gbls->winMisses = 
      WinCreateOffscreenWindow(144, 9, screenFormat, &e); err |= e;

    gbls->winCages = 
      WinCreateOffscreenWindow(175, 31, screenFormat, &e); err |= e;
    gbls->winCageBackup = 
      WinCreateOffscreenWindow(35, 31, screenFormat, &e); err |= e;
    gbls->cageBackupAvailable = false;
    gbls->cageChanged         = true;

    gbls->winKeys = 
      WinCreateOffscreenWindow(112, 22, screenFormat, &e); err |= e;
    gbls->keyChanged      = true;
    gbls->keyOnScreen     = false;
    gbls->keyOldPosition  = 0;

    gbls->winDrops = 
      WinCreateOffscreenWindow(40, 12, screenFormat, &e); err |= e;
    gbls->dropChanged     = true;
    gbls->dropOnScreen    = false;
    gbls->dropOldPosition = 0;

    gbls->winBirds = 
      WinCreateOffscreenWindow(80, 10, screenFormat, &e); err |= e;
    for (i=0; i<MAX_BIRDS; i++) {
      gbls->birdChanged[i]          = true;
      gbls->birdOnScreen[i]         = false;
      gbls->birdOnScreenPosition[i] = 0;
    }

    gbls->winChops = 
      WinCreateOffscreenWindow(130, 10, screenFormat, &e); err |= e;
    for (i=0; i<MAX_CHOPS; i++) {
      gbls->chopChanged[i]          = true;
      gbls->chopOnScreen[i]         = false;
      gbls->chopOnScreenPosition[i] = 0;
    }

    gbls->winKongs = 
      WinCreateOffscreenWindow(140, 128, screenFormat, &e); err |= e;
    gbls->kongChanged     = true;
    gbls->kongOnScreen    = false;
    gbls->kongOldPosition = 0;
  }

  // no problems creating back buffers? load images.
  if (err == errNone) {
  
    WinHandle currWindow;
    MemHandle bitmapHandle;

    currWindow = WinGetDrawWindow();

    // digits
    WinSetDrawWindow(gbls->winDigits);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapDigits);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // misses
    WinSetDrawWindow(gbls->winMisses);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapMisses);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // cage
    WinSetDrawWindow(gbls->winCages);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapCages);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // key
    WinSetDrawWindow(gbls->winKeys);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapKeys);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // drop
    WinSetDrawWindow(gbls->winDrops);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapDrops);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // birds
    WinSetDrawWindow(gbls->winBirds);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapBirds);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // chops
    WinSetDrawWindow(gbls->winChops);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapChops);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    // DK
    WinSetDrawWindow(gbls->winKongs);
    bitmapHandle = DmGet1Resource('Tbmp', bitmapKongs);
    WinDrawBitmap((BitmapType *)MemHandleLock(bitmapHandle), 0, 0);
    MemHandleUnlock(bitmapHandle);
    DmReleaseResource(bitmapHandle);

    WinSetDrawWindow(currWindow);
  }

  result = (err == errNone);

  return result;
}

/**
 * Reset the Game.
 * 
 * @param prefs the global preference data.
 * @param gameType the type of game to configure for.
 */  
void   
GameReset(PreferencesType *prefs, Int8 gameType)
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // turn off all the "bitmaps"
  FrmDrawForm(FrmGetActiveForm());

  // turn on all the "bitmaps"
  {
    RectangleType rect    = { {   0,   0 }, {   0,   0 } };
    RectangleType scrRect = { {   0,   0 }, {   0,   0 } };
    UInt16        i;

    //
    // draw the score
    //

    for (i=0; i<4; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteDigit, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 7;
      scrRect.extent.y  = 12;
      rect.topLeft.x    = 8 * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the digit!
      WinCopyRectangle(gbls->winDigits, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winPaint);
    }

    //
    // draw the misses
    //

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteMiss, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 48;
    scrRect.extent.y  = 9;
    rect.topLeft.x    = 2 * scrRect.extent.x;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // draw the miss bitmap!
    WinCopyRectangle(gbls->winMisses, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    // 
    // draw the cage
    //

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteCage, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 35;
    scrRect.extent.y  = 31;
    rect.topLeft.x    = 0;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // what is the rectangle we need to copy?
    rect.topLeft.x    = 0 * scrRect.extent.x; 
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // draw the cage bitmap!
    WinCopyRectangle(gbls->winCages, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    // what is the rectangle we need to copy?
    rect.topLeft.x    = 4 * scrRect.extent.x; 
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // draw the cage bitmap!
    WinCopyRectangle(gbls->winCages, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    // 
    // draw the keys
    //

    for (i=0; i<4; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteKey, 0, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 28;
      scrRect.extent.y  = 22;
      rect.topLeft.x    = 0;  
      rect.topLeft.y    = 0;  
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // what is the rectangle we need to copy?
      rect.topLeft.x = i * scrRect.extent.x; 
      rect.topLeft.y = 0;
      rect.extent.x  = scrRect.extent.x;
      rect.extent.y  = scrRect.extent.y;

      // draw the key bitmap!
      WinCopyRectangle(gbls->winKeys, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    //
    // draw the drops
    //

    for (i=0; i<4; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteDrop, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 10;
      scrRect.extent.y  = 10;
      rect.topLeft.x    = i * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the drop bitmap!
      WinCopyRectangle(gbls->winDrops, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    //
    // draw the birds
    //

    for (i=0; i<8; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteBird, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 10;
      scrRect.extent.y  = 10;
      rect.topLeft.x    = i * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the bird bitmap!
      WinCopyRectangle(gbls->winBirds, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    // 
    // draw the chops
    //

    for (i=0; i<13; i++) {

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteChop, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 10;
      scrRect.extent.y  = 10;
      rect.topLeft.x    = i * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the chop bitmap!
      WinCopyRectangle(gbls->winChops, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    }

    //
    // draw DKJR
    //

    for (i=0; i<26; i++) {

      // skip #13
      if (i == 13) continue;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteKong, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 20;
      scrRect.extent.y  = 32;
      rect.topLeft.x    = (i % 7) * scrRect.extent.x; 
      rect.topLeft.y    = (i / 7) * scrRect.extent.y; 
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the DK bitmap!
      WinCopyRectangle(gbls->winKongs, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }
  }

  // wait a good two seconds :))
  SysTaskDelay(2 * SysTicksPerSecond());

  // turn off all the "bitmaps"
  FrmDrawForm(FrmGetActiveForm());

  // reset the preferences
  GameResetPreferences(prefs, gameType);
}

/**
 * Reset the Game preferences.
 * 
 * @param prefs the global preference data.
 * @param gameType the type of game to configure for.
 */  
void   
GameResetPreferences(PreferencesType *prefs, Int8 gameType)
{
  GameGlobals *gbls;
  UInt16      i;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // now we are playing
  prefs->game.gamePlaying          = true;
  prefs->game.gamePaused           = false;
  prefs->game.gameWait             = true;
  prefs->game.gameAnimationCount   = 0;

  // reset score and lives
  prefs->game.gameScore            = 0;
  prefs->game.gameLives            = 3;

  // reset dkjr specific things
  prefs->game.dkjr.gameType        = gameType;
  prefs->game.dkjr.gameLevel       = 1;
  prefs->game.dkjr.bonusAvailable  = true;
  prefs->game.dkjr.bonusScoring    = false;

  prefs->game.dkjr.dropWait        = 0;
  prefs->game.dkjr.dropPosition    = 0;

  prefs->game.dkjr.keyHide         = false;
  prefs->game.dkjr.keyDirection    = 1;
  prefs->game.dkjr.keyPosition     = 0;
  prefs->game.dkjr.keyWait         = 0;

  prefs->game.dkjr.dkjrCount       = 0;
  prefs->game.dkjr.dkjrPosition    = 0;
  prefs->game.dkjr.dkjrNewPosition = 0;
  prefs->game.dkjr.dkjrHangWait    = 0;
  prefs->game.dkjr.dkjrJumpWait    = 0;

  prefs->game.dkjr.cageCount       = 4;

  prefs->game.dkjr.chopCount       = 0;
  MemSet(prefs->game.dkjr.chopPosition, sizeof(UInt16) * MAX_CHOPS, 0);
  MemSet(prefs->game.dkjr.chopWait,     sizeof(UInt16) * MAX_CHOPS, 0);

  prefs->game.dkjr.birdCount       = 0;
  MemSet(prefs->game.dkjr.birdPosition, sizeof(UInt16) * MAX_BIRDS, 0);
  MemSet(prefs->game.dkjr.birdWait,     sizeof(UInt16) * MAX_BIRDS, 0);

  // reset the "backup" and "onscreen" flags
  gbls->cageBackupAvailable        = false;
  gbls->cageChanged                = true;
  gbls->keyChanged                 = true;
  gbls->keyOnScreen                = false;
  gbls->dropChanged                = true;
  gbls->dropOnScreen               = false;
  for (i=0; i<MAX_BIRDS; i++) {
    gbls->birdChanged[i]           = true;
    gbls->birdOnScreen[i]          = false;
  }
  for (i=0; i<MAX_CHOPS; i++) {
    gbls->chopChanged[i]           = true;
    gbls->chopOnScreen[i]          = false;
  }
  gbls->kongChanged                = true;
  gbls->kongOnScreen               = false;

  gbls->gameType                   = gameType;
  gbls->playerDied                 = false;
  gbls->moveDelayCount             = 0;
  gbls->moveLast                   = moveNone;
  gbls->moveNext                   = moveNone;
}

/**
 * Process key input from the user.
 * 
 * @param prefs the global preference data.
 * @param keyStatus the current key state.
 */  
void   
GameProcessKeyInput(PreferencesType *prefs, UInt32 keyStatus)
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  keyStatus &= (prefs->config.ctlKeyJump  |
                prefs->config.ctlKeyUp    |
                prefs->config.ctlKeyDown  |
                prefs->config.ctlKeyLeft  |
                prefs->config.ctlKeyRight);

  // additonal checks here
  if (gbls->hardware.gamePadPresent) {

    UInt8 gamePadKeyStatus;
    Err   err;

    // read the state of the gamepad
    err = GPDReadInstant(gbls->hardware.gamePadLibRef, &gamePadKeyStatus);
    if (err == errNone) {

      // process
      if (((gamePadKeyStatus & GAMEPAD_RIGHTFIRE) != 0) ||
          ((gamePadKeyStatus & GAMEPAD_LEFTFIRE)  != 0))
        keyStatus |= prefs->config.ctlKeyJump;
      if  ((gamePadKeyStatus & GAMEPAD_DOWN)      != 0) 
        keyStatus |= prefs->config.ctlKeyDown;
      if  ((gamePadKeyStatus & GAMEPAD_UP)        != 0) 
        keyStatus |= prefs->config.ctlKeyUp;
      if  ((gamePadKeyStatus & GAMEPAD_LEFT)      != 0) 
        keyStatus |= prefs->config.ctlKeyLeft;
      if  ((gamePadKeyStatus & GAMEPAD_RIGHT)     != 0) 
        keyStatus |= prefs->config.ctlKeyRight;

      // special purpose :)
      if  ((gamePadKeyStatus & GAMEPAD_SELECT)    != 0) {

	// wait until they let it go :) 
	do {
          GPDReadInstant(gbls->hardware.gamePadLibRef, &gamePadKeyStatus);
        } while ((gamePadKeyStatus & GAMEPAD_SELECT) != 0);

        keyStatus = 0;
	prefs->game.gamePaused = !prefs->game.gamePaused;
      }
      if  ((gamePadKeyStatus & GAMEPAD_START)     != 0) {
	
	// wait until they let it go :) 
	do {
          GPDReadInstant(gbls->hardware.gamePadLibRef, &gamePadKeyStatus);
        } while ((gamePadKeyStatus & GAMEPAD_START) != 0);

        keyStatus = 0;
        GameReset(prefs, prefs->game.dkjr.gameType);
      }
    }
  }

  // did they press at least one of the game keys?
  if (keyStatus != 0) {

    // if they were waiting, we should reset the game animation count
    if (prefs->game.gameWait) { 
      prefs->game.gameAnimationCount = 0;
      prefs->game.gameWait           = false;
    }

    // great! they wanna play
    prefs->game.gamePaused = false;
  }

  // jump for key
  if (
      ((keyStatus & prefs->config.ctlKeyJump) != 0) &&
      ((keyStatus & prefs->config.ctlKeyLeft) != 0) &&
      (
       (gbls->moveDelayCount == 0) || 
       (gbls->moveLast       != moveJumpKey)
      ) 
     ) {

    // adjust the position if possible
    if (prefs->game.dkjr.dkjrPosition == 18) {
      prefs->game.dkjr.dkjrNewPosition = 20;
    }
  }

  // jump
  else
  if (
      ((keyStatus &  prefs->config.ctlKeyJump) != 0) &&
      (
       (gbls->moveDelayCount == 0) || 
       (gbls->moveLast       != moveJump)
      ) &&
      ((prefs->game.dkjr.dkjrPosition % 2) == 0) // on floor
     ) { 
    
    UInt16 newPosition = prefs->game.dkjr.dkjrPosition + 1;

    // adjust the position if possible
    if ((newPosition >= 0) && (newPosition < 20) && (newPosition != 13)) 
      prefs->game.dkjr.dkjrNewPosition = newPosition;
  }
 
  // move left
  else
  if (
      ((keyStatus &  prefs->config.ctlKeyLeft) != 0) &&
      (
       (gbls->moveDelayCount == 0) || 
       (gbls->moveLast       != moveLeft)
      )
     ) {

    // adjust the position if possible (lower level)
    if ((prefs->game.dkjr.dkjrPosition < 12) &&
        (prefs->game.dkjr.dkjrPosition >  1)
       ) {

      UInt16 newPosition = prefs->game.dkjr.dkjrPosition - 2;

      // adjust the position if possible
      if ((newPosition >= 0) && (newPosition < 20) && (newPosition != 13)) 
        prefs->game.dkjr.dkjrNewPosition = newPosition;
    }

    // adjust the position if possible (upper level)
    else
    if ((prefs->game.dkjr.dkjrPosition > 11) &&
        (prefs->game.dkjr.dkjrPosition < 18)
       ) {

      UInt16 newPosition = prefs->game.dkjr.dkjrPosition + 2;

      // adjust the position if possible
      if ((newPosition >= 0) && (newPosition < 20) && (newPosition != 13)) 
        prefs->game.dkjr.dkjrNewPosition = newPosition;
    }
  }

  // move right
  else
  if (
      ((keyStatus & prefs->config.ctlKeyRight) != 0) &&
      (
       (gbls->moveDelayCount == 0) || 
       (gbls->moveLast       != moveRight)
      )
     ) {

    // adjust the position if possible (lower level)
    if ((prefs->game.dkjr.dkjrPosition < 12) &&
        (prefs->game.dkjr.dkjrPosition < 10)
       ) {

      UInt16 newPosition = prefs->game.dkjr.dkjrPosition + 2;

      // adjust the position if possible
      if ((newPosition >= 0) && (newPosition < 20) && (newPosition != 13)) 
        prefs->game.dkjr.dkjrNewPosition = newPosition;
    }

    // adjust the position if possible (upper level)
    else
    if ((prefs->game.dkjr.dkjrPosition > 11) &&
        (prefs->game.dkjr.dkjrPosition > 13)
       ) {

      UInt16 newPosition = prefs->game.dkjr.dkjrPosition - 2;

      // adjust the position if possible
      if ((newPosition >= 0) && (newPosition < 20) && (newPosition != 13)) 
        prefs->game.dkjr.dkjrNewPosition = newPosition;
    }
  }

  // move up
  else
  if (
      (
       (
        ((keyStatus &  prefs->config.ctlKeyJump) != 0) &&
        (
         (gbls->moveDelayCount == 0) || 
         (gbls->moveLast       != moveJump)
        )
       ) ||
       (
        ((keyStatus &    prefs->config.ctlKeyUp) != 0) && 
        (
         (gbls->moveDelayCount == 0) || 
         (gbls->moveLast       != moveUp)
        )
       )
      )
     ) {

    // adjust the position if possible (move from lower to upper)
    if (prefs->game.dkjr.dkjrPosition == 11) { 
      prefs->game.dkjr.dkjrNewPosition = 12;
    }
  }

  // move down
  else
  if (
      ((keyStatus &  prefs->config.ctlKeyDown) != 0) &&
      (
       (gbls->moveDelayCount == 0) || 
       (gbls->moveLast       != moveDown)
      )
     ) {

    // adjust the position if possible (move from upper to lower)
    if (prefs->game.dkjr.dkjrPosition == 12) { 
      prefs->game.dkjr.dkjrNewPosition = 11;
    }

    // adjust the position if possible (in the air?)
    if ((prefs->game.dkjr.dkjrPosition % 2) == 1) { 

      UInt16 newPosition = prefs->game.dkjr.dkjrPosition - 1;

      // adjust the position if possible
      if ((newPosition >= 0) && (newPosition < 20) && (newPosition != 13)) 
        prefs->game.dkjr.dkjrNewPosition = newPosition;
    }
  }
}
  
/**
 * Process stylus input from the user.
 * 
 * @param prefs the global preference data.
 * @param x the x co-ordinate of the stylus event.
 * @param y the y co-ordinate of the stylus event.
 */  
void   
GameProcessStylusInput(PreferencesType *prefs, Coord x, Coord y)
{
  RectangleType rect;
  UInt16        i;

  // lets take a look at all the possible "positions"
  for (i=0; i<21; i++) {

    // get the bounding box of the position
    GameGetSpritePosition(spriteKong, i,
                          &rect.topLeft.x, &rect.topLeft.y);
    rect.extent.x  = 20;
    rect.extent.y  = 20;

    // did they tap inside this rectangle?
    if ((i != 13) && (RctPtInRectangle(x, y, &rect))) {

      // ok, this is where we are going to go :)
      prefs->game.dkjr.dkjrNewPosition = i;

      // if they were waiting, we should reset the game animation count
      if (prefs->game.gameWait) { 
        prefs->game.gameAnimationCount = 0;
        prefs->game.gameWait           = false;
      }

      // great! they wanna play
      prefs->game.gamePaused = false;
      break;                                        // stop looking
    }
  }
}

/**
 * Process the object movement in the game.
 * 
 * @param prefs the global preference data.
 */  
void   
GameMovement(PreferencesType *prefs)
{
  const CustomPatternType erase = { 0,0,0,0,0,0,0,0 };
  const RectangleType     rect  = {{   0,  16 }, { 160, 16 }};

  GameGlobals    *gbls;
  SndCommandType deathSnd = {sndCmdFreqDurationAmp,0, 512,50,sndMaxAmp};
  UInt16         i, j;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  //
  // the game is NOT paused.
  //

  if (!prefs->game.gamePaused) {

    // animate the key
    if (prefs->game.dkjr.keyWait == 0) {
    
      if ((prefs->game.dkjr.keyPosition  == 0) && 
          (prefs->game.dkjr.keyDirection == -1)) 
        prefs->game.dkjr.keyDirection = 1;
      if ((prefs->game.dkjr.keyPosition  == 3) && 
          (prefs->game.dkjr.keyDirection == 1)) 
        prefs->game.dkjr.keyDirection = -1;
    
      prefs->game.dkjr.keyPosition += prefs->game.dkjr.keyDirection;
      prefs->game.dkjr.keyWait      = 2;
  
      gbls->keyChanged = true;
    }
    else {
      prefs->game.dkjr.keyWait--;
    }

    // we must make sure the user is ready for playing 
    if (!prefs->game.gameWait) {

      // we cannot be dead yet :)
      gbls->playerDied = false;

      // are we in bonus mode?
      if ((prefs->game.dkjr.bonusScoring) &&
          (prefs->game.gameAnimationCount % GAME_FPS) < (GAME_FPS >> 1)) {

        Char   str[32];
        FontID currFont = FntGetFont();

        StrCopy(str, "    * BONUS PLAY *    ");
        FntSetFont(boldFont);
        WinDrawChars(str, StrLen(str), 
                     80 - (FntCharsWidth(str, StrLen(str)) >> 1), 19);
        FntSetFont(currFont);
      }
      else {

        // erase the status area
        WinSetPattern(&erase);
        WinFillRectangle(&rect, 0);
      }

      // has the dropping object hit something?
      switch (prefs->game.dkjr.dropPosition) 
      {
        case 1: 
             // upper level (chops)
             {
               i = 0;
               while (i<prefs->game.dkjr.chopCount) {

                 // are they in the right position?
                 if (prefs->game.dkjr.chopPosition[i] == 2) {
            
                   // this is worth 3 points
                   for (j=0; j<3; j++) {
                     GameIncrementScore(prefs);
                   }

                   // we need to remove the chop
                   GameRemoveChop(prefs, i);
                   prefs->game.dkjr.dropWait = 0; // move the drop object 
                 }
                 else i++;
               }
             }
             break;

        case 2:
             // lower level (birds)
             {
               i = 0;
               while (i<prefs->game.dkjr.birdCount) {

                 // are they in the right position?
                 if (prefs->game.dkjr.birdPosition[i] == 4) {
            
                   // this is worth 6 points
                   for (j=0; j<6; j++) {
                     GameIncrementScore(prefs);
                   }

                   // we need to remove the bird
                   GameRemoveBird(prefs, i);
                   prefs->game.dkjr.dropWait = 0; // move the drop object 
                 }
                 else i++;
               }
             }
             break;

        case 3: 
             // lower level (chops)
             {
               i = 0;
               while (i<prefs->game.dkjr.chopCount) {

                 // are they in the right position?
                 if (prefs->game.dkjr.chopPosition[i] == 9) {
            
                   // this is worth 9 points
                   for (j=0; j<9; j++) {
                     GameIncrementScore(prefs);
                   }

                   // we need to remove the chop
                   GameRemoveChop(prefs, i);
                   prefs->game.dkjr.dropWait = 0; // move the drop object 
                 }
                 else i++;
               }
             }
             break;

        case 0: 
        default:
             break;
      }

      // player gets first move
      GameMovePlayer(prefs);

      // enemies move next
      GameMoveBirds(prefs);
      GameMoveChops(prefs);

      // animate the drop
      if (prefs->game.dkjr.dropWait == 0) {

        // it must be at position 1, 2 or 3 to animate
        if ((prefs->game.dkjr.dropPosition > 0) &&
            (prefs->game.dkjr.dropPosition < 4)) {

          prefs->game.dkjr.dropPosition++;
          prefs->game.dkjr.dropWait =
            (gbls->gameType == GAME_A) ? 4 : 3;
  
          gbls->dropChanged = true;
        }
      }
      else {
        prefs->game.dkjr.dropWait--;
      }

      // is it time to upgrade the game?
      if (prefs->game.gameAnimationCount >= 
           ((gbls->gameType == GAME_A) ? 0x17f : 0x100)) {

        prefs->game.gameAnimationCount = 0;
        prefs->game.dkjr.gameLevel++;

        // upgrading of difficulty?
        if (
            (gbls->gameType             == GAME_A) &&
            (prefs->game.dkjr.gameLevel > MAX_BIRDS)
           ) {

          gbls->gameType              = GAME_B;
          prefs->game.dkjr.gameLevel -= 2;  // give em a break :)
        }
      } 

      // has the player died in this frame?
      if (gbls->playerDied) {

        UInt16        index;
        RectangleType rect    = { {   0,   0 }, {   0,   0 } };
        RectangleType scrRect = { {   0,   0 }, {   0,   0 } };

        // erase the old DK bitmap (he could have just come out of a jump)?
        if (gbls->kongOnScreen) {

          index = gbls->kongOldPosition;

          // what is the rectangle we need to copy?
          GameGetSpritePosition(spriteKong, index, 
                                &scrRect.topLeft.x, &scrRect.topLeft.y);
          scrRect.extent.x  = 20;
          scrRect.extent.y  = 32;
          rect.topLeft.x    = (index % 7) * scrRect.extent.x; 
          rect.topLeft.y    = (index / 7) * scrRect.extent.y; 
          rect.extent.x     = scrRect.extent.x;
          rect.extent.y     = scrRect.extent.y;

          // invert the old DK bitmap
          WinCopyRectangle(gbls->winKongs, WinGetDrawWindow(),
                           &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
          gbls->kongOnScreen    = false;
        }

        // play death sound and flash the player
        for (i=0; i<4; i++) {

          index = prefs->game.dkjr.dkjrPosition;

          // what is the rectangle we need to copy?
          GameGetSpritePosition(spriteKong, index, 
                                &scrRect.topLeft.x, &scrRect.topLeft.y);
          scrRect.extent.x  = 20;
          scrRect.extent.y  = 32;
          rect.topLeft.x    = (index % 7) * scrRect.extent.x; 
          rect.topLeft.y    = (index / 7) * scrRect.extent.y; 
          rect.extent.x     = scrRect.extent.x;
          rect.extent.y     = scrRect.extent.y;

          // invert the old DK bitmap
          WinCopyRectangle(gbls->winKongs, WinGetDrawWindow(),
                           &rect, scrRect.topLeft.x, scrRect.topLeft.y, winInvert);
          
          // play the beep sound
          DevicePlaySound(&deathSnd);
          SysTaskDelay(50);
        }

        // lose a life :(
        prefs->game.gameLives--;

        // no more lives left: GAME OVER!!
        if (prefs->game.gameLives == 0) {

          EventType event;

          // return to main screen
          MemSet(&event, sizeof(EventType), 0);
          event.eType            = menuEvent;
          event.data.menu.itemID = gameMenuItemExit;
          EvtAddEventToQueue(&event);

          prefs->game.gamePlaying = false;
        }

        // reset player position and continue game
        else {
          GameAdjustLevel(prefs);
          prefs->game.dkjr.bonusScoring = false;
          prefs->game.gameWait          = true;
        }
      }
    }

    // we have to display "GET READY!"
    else {

      // flash on:
      if ((prefs->game.gameAnimationCount % GAME_FPS) < (GAME_FPS >> 1)) {

        Char   str[32];
        FontID currFont = FntGetFont();

        StrCopy(str, "    * GET READY *    ");
        FntSetFont(boldFont);
        WinDrawChars(str, StrLen(str), 
                     80 - (FntCharsWidth(str, StrLen(str)) >> 1), 19);
        FntSetFont(currFont);
      }

      // flash off:
      else {

        // erase the status area
        WinSetPattern(&erase);
        WinFillRectangle(&rect, 0);
      }
    }

    // update the animation counter
    prefs->game.gameAnimationCount++;
  }

  //
  // the game is paused.
  //

  else {

    Char   str[32];
    FontID currFont = FntGetFont();

    StrCopy(str, "    *  PAUSED  *    ");
    FntSetFont(boldFont);
    WinDrawChars(str, StrLen(str), 
                 80 - (FntCharsWidth(str, StrLen(str)) >> 1), 19);
    FntSetFont(currFont);
  }
}

/**
 * Draw the game on the screen.
 * 
 * @param prefs the global preference data.
 */
void   
GameDraw(PreferencesType *prefs)
{
  GameGlobals   *gbls;
  UInt16        i, index;
  RectangleType rect    = { {   0,   0 }, {   0,   0 } };
  RectangleType scrRect = { {   0,   0 }, {   0,   0 } };

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // 
  // RESTORE BITMAPS ON SCREEN (if required)
  //

  GameDrawRestore(prefs);

  // 
  // DRAW INFORMATION/BITMAPS ON SCREEN
  //

  // draw the score
  {
    UInt16 base;
 
    base = 1000;  // max score (4 digits)
    for (i=0; i<4; i++) {

      index = (prefs->game.gameScore / base) % 10;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteDigit, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 7;
      scrRect.extent.y  = 12;
      rect.topLeft.x    = index * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the digit!
      WinCopyRectangle(gbls->winDigits, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winPaint);
      base /= 10;
    }
  }

  // draw the misses that have occurred :( 
  if (prefs->game.gameLives < 3) {
  
    index = 2 - prefs->game.gameLives;

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteMiss, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 48;
    scrRect.extent.y  = 9;
    rect.topLeft.x    = index * scrRect.extent.x;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // draw the miss bitmap!
    WinCopyRectangle(gbls->winMisses, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
  }

  // no missed, make sure none are shown
  else {
  
    index = 2;  // the miss with *all* three misses

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteMiss, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 48;
    scrRect.extent.y  = 9;
    rect.topLeft.x    = index * scrRect.extent.x;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // erase the three miss bitmap!
    WinCopyRectangle(gbls->winMisses, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
  }

  // draw the cage on the screen (only if it has changed)
  if (gbls->cageChanged) {

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteCage, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 35;
    scrRect.extent.y  = 31;
    rect.topLeft.x    = 0;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // backup the area behind bitmap
    WinCopyRectangle(WinGetDrawWindow(), gbls->winCageBackup,
                     &scrRect, 0, 0, winPaint);
    gbls->cageBackupAvailable = true;

    // what is the rectangle we need to copy?
    index = prefs->game.dkjr.cageCount;
    rect.topLeft.x    = index * scrRect.extent.x; 
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // draw the cage bitmap!
    WinCopyRectangle(gbls->winCages, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    // dont draw until we need to
    gbls->cageChanged = false;
  }

  // draw the key on the screen (only if it has changed)
  if (gbls->keyChanged) {

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteKey, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 28;
    scrRect.extent.y  = 22;
    rect.topLeft.x    = 0;  
    rect.topLeft.y    = 0;  
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // 
    // erase the previous key 
    // 

    if (gbls->keyOnScreen) {

      index = gbls->keyOldPosition;

      // what is the rectangle we need to copy?
      rect.topLeft.x = index * scrRect.extent.x; 
      rect.topLeft.y = 0;
      rect.extent.x  = scrRect.extent.x;
      rect.extent.y  = scrRect.extent.y;

      // erase the key bitmap!
      WinCopyRectangle(gbls->winKeys, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
      gbls->keyOnScreen    = false;
    }

    // 
    // draw the key at the new position
    // 

    // are we supposed to draw it?
    if (!prefs->game.dkjr.keyHide) {

      index = prefs->game.dkjr.keyPosition;

      // what is the rectangle we need to copy?
      rect.topLeft.x = index * scrRect.extent.x; 
      rect.topLeft.y = 0;
      rect.extent.x  = scrRect.extent.x;
      rect.extent.y  = scrRect.extent.y;

      // save this location, record key is onscreen
      gbls->keyOnScreen    = true;
      gbls->keyOldPosition = index;

      // draw the key bitmap!
      WinCopyRectangle(gbls->winKeys, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    // dont draw until we need to
    gbls->keyChanged = false;
  }

  // draw the drop on the screen (only if it has changed)
  if (gbls->dropChanged) {

    // 
    // erase the previous drop 
    // 

    if (gbls->dropOnScreen) {

      index = gbls->dropOldPosition;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteDrop, index, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 10;
      scrRect.extent.y  = 10;
      rect.topLeft.x    = index * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // invert the old drop bitmap
      WinCopyRectangle(gbls->winDrops, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
      gbls->dropOnScreen    = false;
    }

    // 
    // draw the drop at the new position
    // 

    // lets make sure we can see the drop :)
    if (prefs->game.dkjr.dropPosition < 4) {

      index = prefs->game.dkjr.dropPosition;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteDrop, index, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 10;
      scrRect.extent.y  = 10;
      rect.topLeft.x    = index * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // save this location, record drop is onscreen
      gbls->dropOnScreen    = true;
      gbls->dropOldPosition = index;

      // draw the drop bitmap!
      WinCopyRectangle(gbls->winDrops, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);
    }

    // dont draw until we need to
    gbls->dropChanged = false;
  }

  // draw the birds
  for (i=0; i<prefs->game.dkjr.birdCount; i++) {

    // draw the bird on the screen (only if it has changed)
    if (gbls->birdChanged[i]) {

      // 
      // erase the previous bird 
      // 

      if (gbls->birdOnScreen[i]) {

        index = gbls->birdOnScreenPosition[i];

        // what is the rectangle we need to copy?
        GameGetSpritePosition(spriteBird, index, 
                              &scrRect.topLeft.x, &scrRect.topLeft.y);
        scrRect.extent.x  = 10;
        scrRect.extent.y  = 10;
        rect.topLeft.x    = index * scrRect.extent.x;
        rect.topLeft.y    = 0;
        rect.extent.x     = scrRect.extent.x;
        rect.extent.y     = scrRect.extent.y;

        // invert the old bird bitmap
        WinCopyRectangle(gbls->winBirds, WinGetDrawWindow(),
                         &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
        gbls->birdOnScreen[i]    = false;
      }

      // 
      // draw the bird at the new position
      // 

      index = prefs->game.dkjr.birdPosition[i];

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteBird, index, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 10;
      scrRect.extent.y  = 10;
      rect.topLeft.x    = index * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // save this location, record bird is onscreen
      gbls->birdOnScreen[i]         = true;
      gbls->birdOnScreenPosition[i] = index;

      // draw the bird bitmap!
      WinCopyRectangle(gbls->winBirds, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

      // dont draw until we need to
      gbls->birdChanged[i] = false;
    }
  }

  // draw the chops
  for (i=0; i<prefs->game.dkjr.chopCount; i++) {

    // draw the chop on the screen (only if it has changed)
    if (gbls->chopChanged[i]) {

      // 
      // erase the previous chop 
      // 

      if (gbls->chopOnScreen[i]) {

        index = gbls->chopOnScreenPosition[i];

        // what is the rectangle we need to copy?
        GameGetSpritePosition(spriteChop, index, 
                              &scrRect.topLeft.x, &scrRect.topLeft.y);
        scrRect.extent.x  = 10;
        scrRect.extent.y  = 10;
        rect.topLeft.x    = index * scrRect.extent.x;
        rect.topLeft.y    = 0;
        rect.extent.x     = scrRect.extent.x;
        rect.extent.y     = scrRect.extent.y;

        // invert the old chop bitmap
        WinCopyRectangle(gbls->winChops, WinGetDrawWindow(),
                         &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
        gbls->chopOnScreen[i]    = false;
      }

      // 
      // draw the chop at the new position
      // 

      index = prefs->game.dkjr.chopPosition[i];

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteChop, index, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 10;
      scrRect.extent.y  = 10;
      rect.topLeft.x    = index * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // save this location, record chop is onscreen
      gbls->chopOnScreen[i]         = true;
      gbls->chopOnScreenPosition[i] = index;

      // draw the chop bitmap!
      WinCopyRectangle(gbls->winChops, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

      // dont draw until we need to
      gbls->chopChanged[i] = false;
    }
  }

  // draw DK (only if it has changed)
  if (gbls->kongChanged) {

    // 
    // erase the previous DK
    // 

    if (gbls->kongOnScreen) {

      index = gbls->kongOldPosition;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteKong, index, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 20;
      scrRect.extent.y  = 32;
      rect.topLeft.x    = (index % 7) * scrRect.extent.x; 
      rect.topLeft.y    = (index / 7) * scrRect.extent.y; 
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // invert the old DK bitmap
      WinCopyRectangle(gbls->winKongs, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
      gbls->kongOnScreen    = false;
    }

    // 
    // draw DK at the new position
    // 

    index = prefs->game.dkjr.dkjrPosition;

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteKong, index, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 20;
    scrRect.extent.y  = 32;
    rect.topLeft.x    = (index % 7) * scrRect.extent.x; 
    rect.topLeft.y    = (index / 7) * scrRect.extent.y; 
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // save this location, record DK is onscreen
    gbls->kongOnScreen    = true;
    gbls->kongOldPosition = index;

    // draw the DK bitmap!
    WinCopyRectangle(gbls->winKongs, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winOverlay);

    // dont draw until we need to
    gbls->kongChanged = false;
  }
}

/**
 * Get the position of a particular sprite on the screen.
 *
 * @param spriteType the type of sprite.
 * @param index the index required in the sprite position list.
 * @param x the x co-ordinate of the position
 * @param y the y co-ordinate of the position
 */
void
GameGetSpritePosition(UInt8 spriteType, 
                      UInt8 index, 
                      Coord *x, 
                      Coord *y)
{
  switch (spriteType) 
  {
    case spriteDigit: 
         {
           *x = 120 + (index * 9);
           *y = 37;
         }
         break;

    case spriteMiss: 
         {
           *x = 106;
           *y = 51;
         }
         break;

    case spriteCage: 
         {
           *x = 3;
           *y = 37;
         }
         break;

    case spriteKey: 
         {
           *x = 50;
           *y = 41;
         }
         break;

    case spriteDrop: 
         {
           Coord positions[][2] = {
                                   {  84,  62 },
                                   {  84,  71 },
                                   {  85, 100 },
                                   {  84, 117 }
                                 };

           *x = positions[index][0];
           *y = positions[index][1];
         }
         break;

    case spriteBird: 
         {
           Coord positions[][2] = {
                                   {   4,  97 },
                                   {  14, 107 },
                                   {  39, 106 },
                                   {  61, 108 },
                                   {  83, 109 },
                                   { 105, 108 },
                                   { 124, 107 },
                                   { 146, 100 }
                                 };

           *x = positions[index][0];
           *y = positions[index][1];
         }
         break;

    case spriteChop: 
         {
           Coord positions[][2] = {
                                   {  41,  88 },
                                   {  57,  89 },
                                   {  82,  87 },
                                   { 104,  90 },
                                   { 125,  87 },
                                   { 150,  90 },
                                   { 145, 124 },
                                   { 128, 126 },
                                   { 105, 125 },
                                   {  84, 127 },
                                   {  62, 128 },
                                   {  37, 126 },
                                   {  17, 128 }
                                 };

           *x = positions[index][0];
           *y = positions[index][1];
         } 
         break;

    case spriteKong: 
         {
           Coord positions[][2] = {
                                   {  20, 118 },
                                   {  22, 102 },
                                   {  45, 119 },
                                   {  45, 104 },
                                   {  65, 119 },
                                   {  68, 103 },
                                   {  90, 119 },
                                   {  91, 104 },
                                   { 111, 119 },
                                   { 112, 102 },
                                   { 133, 118 },
                                   { 131, 101 },
                                   { 130,  81 },
                                   {   0,   0 },  // not used
                                   { 108,  82 },
                                   { 113,  67 },
                                   {  88,  81 },
                                   {  90,  64 },
                                   {  65,  79 },
                                   {  64,  65 },
                                   {  49,  64 },
                                   {  13,  91 },
                                   {   4, 107 },
                                   {  32,  37 },
                                   {  32,  37 },
                                   {  34,  68 }
                                 };

           *x = positions[index][0];
           *y = positions[index][1];
         }
         break;

    default:
         break;
  }
}

/**
 * Terminate the game.
 */
void   
GameTerminate()
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // unlock the gamepad driver (if available)
  if (gbls->hardware.gamePadPresent) {

    Err    err;
    UInt32 gamePadUserCount;

    err = GPDClose(gbls->hardware.gamePadLibRef, &gamePadUserCount);
    if (gamePadUserCount == 0)
      SysLibRemove(gbls->hardware.gamePadLibRef);
  }

  // clean up windows/memory
  if (gbls->winDigits != NULL) 
    WinDeleteWindow(gbls->winDigits,     false);
  if (gbls->winMisses != NULL) 
    WinDeleteWindow(gbls->winMisses,     false);
  if (gbls->winCages != NULL)
    WinDeleteWindow(gbls->winCages,      false);
  if (gbls->winCageBackup != NULL)
    WinDeleteWindow(gbls->winCageBackup, false);
  if (gbls->winKeys != NULL)
    WinDeleteWindow(gbls->winKeys,       false);
  if (gbls->winDrops != NULL)
    WinDeleteWindow(gbls->winDrops,      false);
  if (gbls->winBirds != NULL)
    WinDeleteWindow(gbls->winBirds,      false); 
  if (gbls->winChops != NULL)
    WinDeleteWindow(gbls->winChops,      false); 
  if (gbls->winKongs != NULL)
    WinDeleteWindow(gbls->winKongs,      false);
  MemPtrFree(gbls);

  // unregister global data
  FtrUnregister(appCreator, ftrGameGlobals);
}

/**
 * Restore the graphics on the screen (as it was before the draw).
 * 
 * @param prefs the global preference data.
 */
static void   
GameDrawRestore(PreferencesType *prefs)
{
  GameGlobals   *gbls;
  RectangleType rect    = { {   0,   0 }, {   0,   0 } };
  RectangleType scrRect = { {   0,   0 }, {   0,   0 } };

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // 
  // RESTORE INFORMATION/BITMAPS ON SCREEN (in reverse order)
  //

  // restore the area behind the cage (if it is available)
  if (gbls->cageChanged &&
      gbls->cageBackupAvailable) {

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteCage, 0, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 35;
    scrRect.extent.y  = 31;
    rect.topLeft.x    = 0;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    WinCopyRectangle(gbls->winCageBackup, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winPaint);
    gbls->cageBackupAvailable = false;
  }
}

/**
 * Adjust the level (remove birds that are too close and reset positions)
 *
 * @param prefs the global preference data.
 */
static void 
GameAdjustLevel(PreferencesType *prefs)
{
  GameGlobals *gbls;
  UInt16      i;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // remove any birds that are too close to start
  i = 0;
  while (i<prefs->game.dkjr.birdCount) {

    UInt8 removalPoint = 2;

    // is the bird too close?
    if (prefs->game.dkjr.birdPosition[i] < removalPoint) {
            
      // we need to remove the bird
      GameRemoveBird(prefs, i);
    }
    else i++;
  }

  // remove any chops that are too close to start
  i = 0;
  while (i<prefs->game.dkjr.chopCount) {

    UInt8 removalPoint = 9;

    // is the chop too close?
    if (prefs->game.dkjr.chopPosition[i] > removalPoint) {
            
      // we need to remove the chop
      GameRemoveChop(prefs, i);
    }
    else i++;
  }

  // return to start position
  prefs->game.dkjr.dkjrCount       = 0;
  prefs->game.dkjr.dkjrPosition    = 0;
  prefs->game.dkjr.dkjrNewPosition = 0;
  prefs->game.dkjr.dkjrJumpWait    = 0;
  prefs->game.dkjr.dropPosition    = 0;
  gbls->kongChanged                = true;
  gbls->dropChanged                = true;

  // player is not dead :))
  gbls->playerDied                 = false;
}

/**
 * Increment the players score. 
 *
 * @param prefs the global preference data.
 */
static void 
GameIncrementScore(PreferencesType *prefs)
{
  GameGlobals    *gbls;
  UInt16         i, index;
  RectangleType  rect     = { {   0,   0 }, {   0,   0 } };
  RectangleType  scrRect  = { {   0,   0 }, {   0,   0 } };
  SndCommandType scoreSnd = {sndCmdFreqDurationAmp,0,1024, 5,sndMaxAmp};

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // adjust accordingly
  prefs->game.gameScore += prefs->game.dkjr.bonusScoring ? 2 : 1;

  // redraw score bitmap
  {
    UInt16 base;
 
    base = 1000;  // max score (4 digits)
    for (i=0; i<4; i++) {

      index = (prefs->game.gameScore / base) % 10;

      // what is the rectangle we need to copy?
      GameGetSpritePosition(spriteDigit, i, 
                            &scrRect.topLeft.x, &scrRect.topLeft.y);
      scrRect.extent.x  = 7;
      scrRect.extent.y  = 12;
      rect.topLeft.x    = index * scrRect.extent.x;
      rect.topLeft.y    = 0;
      rect.extent.x     = scrRect.extent.x;
      rect.extent.y     = scrRect.extent.y;

      // draw the digit!
      WinCopyRectangle(gbls->winDigits, WinGetDrawWindow(),
                       &rect, scrRect.topLeft.x, scrRect.topLeft.y, winPaint);
      base /= 10;
    }
  }

  // play the sound
  DevicePlaySound(&scoreSnd);
  SysTaskDelay(5);

  // is it time for a bonus?
  if (
      (prefs->game.gameScore >= 300) &&
      (prefs->game.dkjr.bonusAvailable)
     ) {

    SndCommandType snd = {sndCmdFreqDurationAmp,0,0,5,sndMaxAmp};

    // give a little fan-fare sound
    for (i=0; i<15; i++) {
      snd.param1 += 256 + (1 << i);  // frequency
      DevicePlaySound(&snd);

      SysTaskDelay(2); // small deley 
    }

    // apply the bonus!
    if (prefs->game.gameLives == 3) 
      prefs->game.dkjr.bonusScoring = true;
    else
      prefs->game.gameLives = 3;

    prefs->game.dkjr.bonusAvailable = false;
  }
}

/**
 * Move the player.
 *
 * @param prefs the global preference data.
 */
static void
GameMovePlayer(PreferencesType *prefs) 
{
  GameGlobals    *gbls;
  SndCommandType plymvSnd = {sndCmdFreqDurationAmp,0, 768, 5,sndMaxAmp};
  UInt16         i;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // DK in normal gameplay?
  if (prefs->game.dkjr.dkjrPosition < 20) {

    //
    // where does DK want to go today?
    //

    // current position differs from new position?
    if (prefs->game.dkjr.dkjrPosition != prefs->game.dkjr.dkjrNewPosition) {

      // bottom level: 
      if (prefs->game.dkjr.dkjrPosition < 12) {

        // special case, jump to upper level
        if ((prefs->game.dkjr.dkjrPosition    == 11) && 
            (prefs->game.dkjr.dkjrNewPosition  > 11)) {
          gbls->moveNext = moveUp;
        }

        // special case, at end and must jump to position 11
        else
        if ((prefs->game.dkjr.dkjrPosition    == 10) && 
            (prefs->game.dkjr.dkjrNewPosition >= 11)) {
          gbls->moveNext = moveJump;
        }

        // move left:
        else
        if ((prefs->game.dkjr.dkjrPosition    >> 1) > 
            (prefs->game.dkjr.dkjrNewPosition >> 1)) {
          gbls->moveNext = moveLeft;
        }

        // move right:
        else
        if ((prefs->game.dkjr.dkjrPosition    >> 1) < 
            (prefs->game.dkjr.dkjrNewPosition >> 1)) {
          gbls->moveNext = moveRight;
        }

        // jump up:
        else
        if (((prefs->game.dkjr.dkjrNewPosition % 2) == 1) &&
            ((prefs->game.dkjr.dkjrPosition    % 2) == 0)) {
          gbls->moveNext = moveJump;
        }

        // move down:
        else 
        if (((prefs->game.dkjr.dkjrNewPosition % 2) == 0) &&
            ((prefs->game.dkjr.dkjrPosition    % 2) == 1)) {
          gbls->moveNext = moveDown;
        }
      }

      // upper level:
      else {

        // special case: at "hiding spot" - one place to go :))
        if (prefs->game.dkjr.dkjrPosition     == 15) {
          gbls->moveNext = moveDown;
        }

        // special case, move down to lower level
        else
        if ((prefs->game.dkjr.dkjrPosition    == 12) && 
            (prefs->game.dkjr.dkjrNewPosition  < 12)) {
          gbls->moveNext = moveDown;
        }

        // special case, jump for key!
        else
        if ((prefs->game.dkjr.dkjrPosition    == 18) && 
            (prefs->game.dkjr.dkjrNewPosition == 20)) {
          gbls->moveNext = moveJumpKey;
        }

        // move left:
        else
        if ((prefs->game.dkjr.dkjrPosition    >> 1) < 
            (prefs->game.dkjr.dkjrNewPosition >> 1)) {
          gbls->moveNext = moveLeft;
        }

        // move right:
        else
        if ((prefs->game.dkjr.dkjrPosition    >> 1) > 
            (prefs->game.dkjr.dkjrNewPosition >> 1)) {
          gbls->moveNext = moveRight;
        }

        // jump up:
        else
        if (((prefs->game.dkjr.dkjrNewPosition % 2) == 1) &&
            ((prefs->game.dkjr.dkjrPosition    % 2) == 0)) {
          gbls->moveNext = moveJump;
        }

        // move down:
        else 
        if (((prefs->game.dkjr.dkjrNewPosition % 2) == 0) &&
            ((prefs->game.dkjr.dkjrPosition    % 2) == 1)) {
          gbls->moveNext = moveDown;
        }
      }
    }

    // lets make sure they are allowed to do the move
    if (
        (gbls->moveDelayCount == 0) || 
        (gbls->moveLast != gbls->moveNext) 
       ) {
      gbls->moveDelayCount = 
       ((gbls->gameType == GAME_A) ? 4 : 3);
    }
    else {
      gbls->moveDelayCount--;
      gbls->moveNext = moveNone;
    }

    //
    // move DK into the right position based on his desired move
    //

    // is the player suspended in the air/hanging on vine?
    if ((prefs->game.dkjr.dkjrPosition % 2) == 1) {

      // has DK been on vine/in air too long?
      if (prefs->game.dkjr.dkjrHangWait == 0) {

        // lets make sure they dont jump back up :)
        if (prefs->game.dkjr.dkjrNewPosition == prefs->game.dkjr.dkjrPosition)
          prefs->game.dkjr.dkjrNewPosition = prefs->game.dkjr.dkjrPosition - 1;

        // automagically move down :)
        gbls->moveNext = moveDown;
      }
      else {
        prefs->game.dkjr.dkjrHangWait--;
      }

      // has DK just completed a jump over a chop?
      if (
          ((prefs->game.dkjr.dkjrPosition % 2) == 1) &&
          (prefs->game.dkjr.dkjrJumpWait == 1)
         ) {

        for (i=0; i<prefs->game.dkjr.chopCount; i++) {

          if ((
               (prefs->game.dkjr.dkjrPosition > 11) &&        // upper level
               (prefs->game.dkjr.dkjrPosition < 20) &&        
               (gbls->moveLast != moveRight)        &&
               ((((prefs->game.dkjr.dkjrPosition-1) / 2) - 6) == 
                 (5 - prefs->game.dkjr.chopPosition[i]))      // chop position
              ) ||
              (
               (prefs->game.dkjr.dkjrPosition < 12) &&        // lower level
               (gbls->moveLast != moveLeft)         &&
               (((prefs->game.dkjr.dkjrPosition-1) / 2) == 
                 (12 - prefs->game.dkjr.chopPosition[i]))     // chop position
              )) { 

            // increase score
            GameIncrementScore(prefs);
          }
        }
      }
    }
  
    // update counter
    prefs->game.dkjr.dkjrCount++;

    // can the player move?
    if (prefs->game.dkjr.dkjrJumpWait == 0) {

      // which direction do they wish to move?
      switch (gbls->moveNext)
      {
        case moveLeft:

             // lower level:
             if (prefs->game.dkjr.dkjrPosition < 12) {

               // lets make sure they can move left
               if (prefs->game.dkjr.dkjrPosition > 1) {

                 // step out into air:
                 if ((prefs->game.dkjr.dkjrPosition % 2) == 1) {
                 
                   // did DK move into a bird?
                   for (i=0; i<prefs->game.dkjr.birdCount; i++) {

                     gbls->playerDied |= 
                       (
                        ((prefs->game.dkjr.dkjrPosition-2) >> 1) == 
                        (prefs->game.dkjr.birdPosition[i]-2)
                       );
                   }

                   // DK did a valid move (no death)
                   if (!gbls->playerDied) {

                     prefs->game.dkjr.dkjrPosition -= 2;
                     gbls->kongChanged              = true;

                     // no vine available:
                     if ((prefs->game.dkjr.dkjrPosition == 3) ||
                         (prefs->game.dkjr.dkjrPosition == 9)) {
                       prefs->game.dkjr.dkjrJumpWait = 4;
                       prefs->game.dkjr.dkjrHangWait = 
                         prefs->game.dkjr.dkjrJumpWait;
                     } 

                     // vine available:
                     else {
                       prefs->game.dkjr.dkjrJumpWait = 0;
                       prefs->game.dkjr.dkjrHangWait = 64;
                     }
                   }
                 }

                 // walking on the ground:
                 else {

                   // did DK move into a chop?
                   for (i=0; i<prefs->game.dkjr.chopCount; i++) {

                     gbls->playerDied |= 
                       (
                        ((prefs->game.dkjr.dkjrPosition >> 1) == 
                        (12 - prefs->game.dkjr.chopPosition[i]))
                       );
                   }

                   // DK did a valid move (no death)
                   if (!gbls->playerDied) {

                     prefs->game.dkjr.dkjrPosition -= 2;
                     gbls->kongChanged              = true;
                   }
                 }
               }
             }

             // upper level:
             else {

               // lets make sure they can move left
               if ((prefs->game.dkjr.dkjrPosition < 18) &&
                   ((prefs->game.dkjr.dkjrPosition % 2) == 0)) {

                 // did DK move into a chop?
                 for (i=0; i<prefs->game.dkjr.chopCount; i++) {

                   gbls->playerDied |= 
                     (
                      (((prefs->game.dkjr.dkjrPosition+2) >> 1) - 6) == 
                      (5 - prefs->game.dkjr.chopPosition[i])
                     );
                 }

                 // DK did a valid move (no death)
                 if (!gbls->playerDied) {

                   prefs->game.dkjr.dkjrPosition += 2;
                   gbls->kongChanged              = true;
                 }
               }
             }
             break;

        case moveRight:

             // lower level:
             if (prefs->game.dkjr.dkjrPosition < 12) {

               // lets make sure they can move right
               if (prefs->game.dkjr.dkjrPosition < 10) {

                 // step out into air:
                 if ((prefs->game.dkjr.dkjrPosition % 2) == 1) {
                 
                   // did DK move into a bird?
                   for (i=0; i<prefs->game.dkjr.birdCount; i++) {

                     gbls->playerDied |= 
                       (
                        (prefs->game.dkjr.dkjrPosition >> 1) == 
                        (prefs->game.dkjr.birdPosition[i]-2)
                       );
                   }

                   // DK did a valid move (no death)
                   if (!gbls->playerDied) {

                     prefs->game.dkjr.dkjrPosition += 2;
                     gbls->kongChanged              = true;

                     // no vine available:
                     if ((prefs->game.dkjr.dkjrPosition == 3) ||
                         (prefs->game.dkjr.dkjrPosition == 9)) {
                       prefs->game.dkjr.dkjrJumpWait = 4;
                       prefs->game.dkjr.dkjrHangWait = 
                         prefs->game.dkjr.dkjrJumpWait;
                     } 

                     // vine available:
                     else {
                       prefs->game.dkjr.dkjrJumpWait = 0;
                       prefs->game.dkjr.dkjrHangWait = 64;
                     }
                   }
                 }

                 // walking on the ground:
                 else {

                   // did DK move into a chop?
                   for (i=0; i<prefs->game.dkjr.chopCount; i++) {

                     gbls->playerDied |= 
                       (
                        ((prefs->game.dkjr.dkjrPosition+2) >> 1) == 
                        (12 - prefs->game.dkjr.chopPosition[i])
                       );
                   }

                   // DK did a valid move (no death)
                   if (!gbls->playerDied) {

                     prefs->game.dkjr.dkjrPosition += 2;
                     gbls->kongChanged              = true;
                   }
                 }
               }
             }

             // upper level:
             else {

               // lets make sure they can move right
               if ((prefs->game.dkjr.dkjrPosition > 12) &&
                   ((prefs->game.dkjr.dkjrPosition % 2) == 0)) {

                 // did DK move into a chop?
                 for (i=0; i<prefs->game.dkjr.chopCount; i++) {

                   gbls->playerDied |= 
                     (
                      ((prefs->game.dkjr.dkjrPosition / 2) - 6) == 
                      (5 - prefs->game.dkjr.chopPosition[i])
                     );
                 }

                 // DK did a valid move (no death)
                 if (!gbls->playerDied) {
                   prefs->game.dkjr.dkjrPosition -= 2;
                   gbls->kongChanged              = true;
                 }
               }
             }
             break;

        case moveJump:
             // we can only jump if we are on the ground
             if (
                 ((prefs->game.dkjr.dkjrPosition % 2) == 0) &&
                  (prefs->game.dkjr.dkjrPosition != 12)
                ) {

               prefs->game.dkjr.dkjrPosition++;
               gbls->kongChanged = true;

               // DK jumped into air?
               if ((prefs->game.dkjr.dkjrPosition == 3)  ||
                   (prefs->game.dkjr.dkjrPosition == 9)  ||
                   (prefs->game.dkjr.dkjrPosition == 17) ||
                   (prefs->game.dkjr.dkjrPosition == 19)) {
                 prefs->game.dkjr.dkjrJumpWait = 4;
                 prefs->game.dkjr.dkjrHangWait = prefs->game.dkjr.dkjrJumpWait;
               }

               // DK landed on a vine
               else {
                 prefs->game.dkjr.dkjrJumpWait = 0;
                 prefs->game.dkjr.dkjrHangWait = 64;
               }
             }
             break;

        case moveJumpKey:

             // go for it!
             prefs->game.dkjr.dkjrJumpWait =
               (gbls->gameType == GAME_A) ? 3 : 2;
             prefs->game.dkjr.dkjrHangWait = prefs->game.dkjr.dkjrJumpWait;
             prefs->game.dkjr.dkjrPosition = 20;
             gbls->kongChanged             = true;
             break;

        case moveUp:

             // we can only move up at pos=11
             if (prefs->game.dkjr.dkjrPosition == 11) {
               prefs->game.dkjr.dkjrPosition++;
               gbls->kongChanged = true;
             }
             break;

        case moveDown:

             // we can only move down if at pos=12, or are off ground
             if ((prefs->game.dkjr.dkjrPosition == 12) ||
                 ((prefs->game.dkjr.dkjrPosition % 2) == 1)) {

               prefs->game.dkjr.dkjrPosition--;
               gbls->kongChanged = true;

               // DK landed on a vine
               if (prefs->game.dkjr.dkjrPosition == 11) {
                 prefs->game.dkjr.dkjrHangWait = 64;
               }
             }
             break;

        default:
             break;
      }

      gbls->moveLast = gbls->moveNext;
      gbls->moveNext = moveNone;
    }
    else {
      prefs->game.dkjr.dkjrJumpWait--;
    }
  }

  // DK reaching for key? falling?
  else {

    Boolean grabbedKey = false;

    // lets determine if DK grabbed key
    grabbedKey = (
                  (prefs->game.dkjr.dkjrPosition == 20) &&
                  (prefs->game.dkjr.keyPosition  == 0)  && 
                  (prefs->game.dkjr.keyWait      <  2) // must be there 1 frame
                 );
    
    // DK did not grab key :(
    if (!grabbedKey) {

      // can the player move?
      if (prefs->game.dkjr.dkjrJumpWait == 0) {

        switch (prefs->game.dkjr.dkjrPosition)
        {
          case 22: // DK missed? 
               gbls->playerDied = true;
               break;

          case 25: // DK is done :)
               GameAdjustLevel(prefs);

               // has the cage been opened?
               if (prefs->game.dkjr.cageCount == 0) {

                 GameAdjustmentType adjustType;

                 // define the adjustment
                 adjustType.adjustMode = gameCageReset;

                 // reset the cage:
                 if (RegisterAdjustGame(prefs, &adjustType)) {
                   gbls->cageChanged = true;
                 }

#ifdef PROTECTION_ON
                 // terminate the game:
                 else {

                   EventType event;

                   // "please register" dialog :)
                   ApplicationDisplayDialog(rbugForm);

                   // GAME OVER - return to main screen
                   MemSet(&event, sizeof(EventType), 0);
                   event.eType            = menuEvent;
                   event.data.menu.itemID = gameMenuItemExit;
                   EvtAddEventToQueue(&event);

                   // stop the game in its tracks
                   prefs->game.gamePlaying = false;
                 }
#endif
               }
               prefs->game.dkjr.keyHide = false;
               gbls->keyChanged         = true;
               break;

          case 24: // DK opening cage!
               {
                 UInt16 i, bonus, ticks;

                 // release a lock
                 prefs->game.dkjr.cageCount--;
                 gbls->cageChanged = true;
                 bonus = (prefs->game.dkjr.cageCount == 0) ? 20 : 0;

                 // how many half seconds did it take?
                 ticks = (prefs->game.dkjr.dkjrCount / (GAME_FPS >> 1));
 
                 // < 2.5 secs +20, > 10 secs +5, between? 5..20 :)
                 if (ticks < 5)  bonus += 20; else
                 if (ticks > 20) bonus += 5;  else
                                 bonus += 5 + (20 - ticks);

                 // apply the bonus
                 for (i=0; i<bonus; i++) {
                   GameIncrementScore(prefs);
                 }
               }  

          default:
               prefs->game.dkjr.dkjrPosition++;
               prefs->game.dkjr.dkjrJumpWait = 4;
               gbls->kongChanged             = true;
               break;
        }
      }
      else {
        prefs->game.dkjr.dkjrJumpWait--;
      }
    }

    // DK did grab key :)
    else {

      prefs->game.dkjr.dkjrPosition = 23;   // skip forward
      prefs->game.dkjr.dkjrJumpWait = 4;
      prefs->game.dkjr.keyHide      = true;
      gbls->kongChanged             = true;
      gbls->keyChanged              = true;
    }
  }

  // do we need to play a movement sound? 
  if (gbls->kongChanged)  
    DevicePlaySound(&plymvSnd);
  
  // has the player jumped near the drop object, and it is available?
  if ((prefs->game.dkjr.dkjrPosition == 17) &&
      (prefs->game.dkjr.dropPosition == 0)) {

    prefs->game.dkjr.dropPosition++;
    prefs->game.dkjr.dropWait =
      (gbls->gameType == GAME_A) ? 4 : 3;

    gbls->dropChanged = true;
  }
}

/**
 * Move the birds.
 *
 * @param prefs the global preference data.
 */
static void
GameMoveBirds(PreferencesType *prefs) 
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // only do this if the player is still alive
  if (!gbls->playerDied) {

    SndCommandType grdmvSnd = {sndCmdFreqDurationAmp,0, 384, 5,sndMaxAmp};
    UInt16         i, j;

    // process all birds available
    i = 0;
    while (i<prefs->game.dkjr.birdCount) {

      Boolean removal = false;

      if (prefs->game.dkjr.birdWait[i] == 0) {

        Boolean ok;

        // lets make sure it is not moving into a bird in front of us?
        ok = true;
        for (j=0; j<prefs->game.dkjr.birdCount; j++) {
          ok &= (
                 (prefs->game.dkjr.birdPosition[i]+1 != 
                  prefs->game.dkjr.birdPosition[j])
                );
        }

        // the coast is clear, move!
        if (ok) {

          prefs->game.dkjr.birdPosition[i]++;
          prefs->game.dkjr.birdWait[i] = 
            (gbls->gameType == GAME_A) ? 6 : 5;
  
          // the bird has disapeared offscreen:
          if (prefs->game.dkjr.birdPosition[i] > (MAX_BIRDS-1)) {
            GameRemoveBird(prefs, i); removal = true;
          }
  
          // the bird has just moved position onscreen:
          else {
  
            // has it killed the player?
            gbls->playerDied |= 
              (
                (prefs->game.dkjr.dkjrPosition < 12) &&        // lower level
                ((prefs->game.dkjr.dkjrPosition % 2) == 1) &&  // off the ground
                ((prefs->game.dkjr.dkjrPosition / 2) == 
                   (prefs->game.dkjr.birdPosition[i]-2))       // bird position
              );
    
            gbls->birdChanged[i] = true;
          }
    
          DevicePlaySound(&grdmvSnd);
        }
      }
      else {
        prefs->game.dkjr.birdWait[i]--;
      }

      if (!removal) i++;
    }

    // new bird appearing on screen?
    {
      Boolean ok;
      UInt8   birthFactor      = (gbls->gameType == GAME_A) ? 8 : 4;
      UInt8   maxOnScreenBirds = (1 + ((prefs->game.dkjr.gameLevel-1) >> 1));
  
      // we must be able to add a bird (based on level)
      ok = (
            (prefs->game.dkjr.birdCount < maxOnScreenBirds) &&
            (prefs->game.dkjr.birdCount < MAX_BIRDS) &&
            ((SysRandom(0) % birthFactor) == 0)
           );
  
      // lets check that there are no birds at index=0
      for (i=0; i<prefs->game.dkjr.birdCount; i++) {
        ok &= (prefs->game.dkjr.birdPosition[i] != 0);
      }
  
      // lets add a new bird :) 
      if (ok) {
        i = prefs->game.dkjr.birdCount++;
        prefs->game.dkjr.birdPosition[i] = 0;
        prefs->game.dkjr.birdWait[i]     =
          (gbls->gameType == GAME_A) ? 6 : 5;
        gbls->birdChanged[i]             = true;
        gbls->birdOnScreen[i]            = false;
        gbls->birdOnScreenPosition[i]    = 0;
      }
    }
  }
}

/**
 * Remove a bird from the game.
 *
 * @param prefs the global preference data.
 * @param birdIndex the index of the bird to remove.
 */
static void
GameRemoveBird(PreferencesType *prefs,  
               UInt16          birdIndex) 
{
  GameGlobals   *gbls;
  UInt16        index;
  RectangleType rect    = { {   0,   0 }, {   0,   0 } };
  RectangleType scrRect = { {   0,   0 }, {   0,   0 } };

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  //
  // remove the bitmap from the screen
  //

  if (gbls->birdOnScreen[birdIndex]) {

    index = gbls->birdOnScreenPosition[birdIndex];

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteBird, index, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 10;
    scrRect.extent.y  = 10;
    rect.topLeft.x    = index * scrRect.extent.x;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // invert the old bird bitmap
    WinCopyRectangle(gbls->winBirds, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
  }
  gbls->birdOnScreen[birdIndex]         = false;
  gbls->birdOnScreenPosition[birdIndex] = 0;     // ensure this is clean

  // we will push the 'bird' out of the array
  //
  // before: 1234567---  after: 1345672---
  //          ^     |                 |
  //                end point         end point

  prefs->game.dkjr.birdCount--;

  // removal NOT from end?
  if (prefs->game.dkjr.birdCount > birdIndex) {

    UInt16 i, count;

    count = prefs->game.dkjr.birdCount - birdIndex;

    // shift all elements down
    for (i=birdIndex; i<(birdIndex+count); i++) {
      prefs->game.dkjr.birdPosition[i] = prefs->game.dkjr.birdPosition[i+1];
      prefs->game.dkjr.birdWait[i]     = prefs->game.dkjr.birdWait[i+1];
      gbls->birdChanged[i]             = gbls->birdChanged[i+1];
      gbls->birdOnScreen[i]            = gbls->birdOnScreen[i+1];
      gbls->birdOnScreenPosition[i]    = gbls->birdOnScreenPosition[i+1];
    }
  }
}

/**
 * Move the chops.
 *
 * @param prefs the global preference data.
 */
static void
GameMoveChops(PreferencesType *prefs) 
{
  GameGlobals *gbls;

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  // only do this if the player is still alive
  if (!gbls->playerDied) {

    SndCommandType grdmvSnd = {sndCmdFreqDurationAmp,0, 384, 5,sndMaxAmp};
    UInt16         i, j;

    // process all chops available
    i = 0;
    while (i<prefs->game.dkjr.chopCount) {
  
      Boolean removal = false;

      if (prefs->game.dkjr.chopWait[i] == 0) {
  
        Boolean ok;

        // lets make sure it is not moving into a chop in front of us?
        ok = true;
        for (j=0; j<prefs->game.dkjr.chopCount; j++) {
          ok &= (
                 (prefs->game.dkjr.chopPosition[i]+1 != 
                  prefs->game.dkjr.chopPosition[j])
                );
        }

        // the coast is clear, move!
        if (ok) {

          prefs->game.dkjr.chopPosition[i]++;
          prefs->game.dkjr.chopWait[i] = 
            (gbls->gameType == GAME_A) ? 6 : 4;
  
          // the chop has disapeared offscreen:
          if (prefs->game.dkjr.chopPosition[i] > (MAX_CHOPS-1)) {
            GameRemoveChop(prefs, i); removal = true;
          }
  
          // the chop has just moved position onscreen:
          else {
  
            // has it killed the player?
            gbls->playerDied |= 
              (
               (
                (prefs->game.dkjr.dkjrPosition > 11) &&        // upper level
                (prefs->game.dkjr.dkjrPosition < 20) &&        
                ((prefs->game.dkjr.dkjrPosition % 2) == 0) &&  // on the ground
                (((prefs->game.dkjr.dkjrPosition / 2) - 6) == 
                  (5 - prefs->game.dkjr.chopPosition[i]))      // chop position
               ) ||
               (
                (prefs->game.dkjr.dkjrPosition < 12) &&        // lower level
                ((prefs->game.dkjr.dkjrPosition % 2) == 0) &&  // on the ground
                ((prefs->game.dkjr.dkjrPosition / 2) == 
                  (12 - prefs->game.dkjr.chopPosition[i]))     // chop position
               )
              );

            gbls->chopChanged[i] = true;
          }
    
          DevicePlaySound(&grdmvSnd);
        }
      }
      else {
        prefs->game.dkjr.chopWait[i]--;
      }

      if (!removal) i++;
    }

    // new chop appearing on screen?
    {
      Boolean ok;
      UInt8   birthFactor      = (gbls->gameType == GAME_A) ? 8 : 4;
      UInt8   maxOnScreenChops = prefs->game.dkjr.gameLevel;
  
      // we must be able to add a chop (based on level)
      ok = (
            (prefs->game.dkjr.chopCount < maxOnScreenChops) &&
            (prefs->game.dkjr.chopCount < MAX_CHOPS) &&
            ((SysRandom(0) % birthFactor) == 0)
           );
  
      // lets check that there are no chops at index=0
      for (i=0; i<prefs->game.dkjr.chopCount; i++) {
        ok &= (prefs->game.dkjr.chopPosition[i] != 0);
      }

      // lets add a new chop :) 
      if (ok) {
        i = prefs->game.dkjr.chopCount++;
        prefs->game.dkjr.chopPosition[i] = 0;
        prefs->game.dkjr.chopWait[i]     =
          (gbls->gameType == GAME_A) ? 6 : 4;
        gbls->chopChanged[i]             = true;
        gbls->chopOnScreen[i]            = false;
        gbls->chopOnScreenPosition[i]    = 0;
      }
    }
  }
}

/**
 * Remove a chop from the game.
 *
 * @param prefs the global preference data.
 * @param chopIndex the index of the chop to remove.
 */
static void
GameRemoveChop(PreferencesType *prefs, 
               UInt16          chopIndex) 
{
  GameGlobals   *gbls;
  UInt16        index;
  RectangleType rect    = { {   0,   0 }, {   0,   0 } };
  RectangleType scrRect = { {   0,   0 }, {   0,   0 } };

  // get a globals reference
  FtrGet(appCreator, ftrGameGlobals, (UInt32 *)&gbls);

  //
  // remove the bitmap from the screen
  //

  if (gbls->chopOnScreen[chopIndex]) {

    index = gbls->chopOnScreenPosition[chopIndex];

    // what is the rectangle we need to copy?
    GameGetSpritePosition(spriteChop, index, 
                          &scrRect.topLeft.x, &scrRect.topLeft.y);
    scrRect.extent.x  = 10;
    scrRect.extent.y  = 10;
    rect.topLeft.x    = index * scrRect.extent.x;
    rect.topLeft.y    = 0;
    rect.extent.x     = scrRect.extent.x;
    rect.extent.y     = scrRect.extent.y;

    // invert the old chop bitmap
    WinCopyRectangle(gbls->winChops, WinGetDrawWindow(),
                     &rect, scrRect.topLeft.x, scrRect.topLeft.y, winMask);
  }
  gbls->chopOnScreen[chopIndex]         = false;
  gbls->chopOnScreenPosition[chopIndex] = 0;     // ensure this is clean

  //
  // update the information arrays
  //

  // we will push the 'chop' out of the array
  //
  // before: 1234567---  after: 1345672---
  //          ^     |                 |
  //                end point         end point

  prefs->game.dkjr.chopCount--;

  // removal NOT from end?
  if (prefs->game.dkjr.chopCount > chopIndex) {

    UInt16 i, count;

    count = prefs->game.dkjr.chopCount - chopIndex;

    // shift all elements down
    for (i=chopIndex; i<(chopIndex+count); i++) {
      prefs->game.dkjr.chopPosition[i] = prefs->game.dkjr.chopPosition[i+1];
      prefs->game.dkjr.chopWait[i]     = prefs->game.dkjr.chopWait[i+1];
      gbls->chopChanged[i]             = gbls->chopChanged[i+1];
      gbls->chopOnScreen[i]            = gbls->chopOnScreen[i+1];
      gbls->chopOnScreenPosition[i]    = gbls->chopOnScreenPosition[i+1];
    }
  }
}
