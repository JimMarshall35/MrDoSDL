2000 const EnemyWaitTimeBeforeBecomeDigger

1000 const EnemyFlashTime

0 const EnemyTypeNormal

1 const EnemyTypeTurningIntoDigger

2 const EnemyTypeDigger

3 const EnemyTypeExtraMan

4 const EnemyTypeGhost

: OnReachedPathEnd ( enemy )
	GetCharacterTile SetPathTo
;

: IncrementDeltaTime ( enemyPtr -- enemyPtr )
	dup
	GetTimerPtr @ GetDeltaT + ( enemyPtr timerValue+deltaT )
	swap dup                  ( timerValue+deltaT enemyPtr enemyPtr )
	GetTimerPtr               ( timerValue+deltaT enemyPtr timerPtr )
	rot                       ( enemyPtr timerPtr timerValue+deltaT )
	swap !
;

: TransformToDigger ( enemyPtr -- )
	dup GetEnemyTypePtr                 ( enemy enemyTypePtr )
	EnemyTypeTurningIntoDigger swap !32 ( enemy )
	dup GetTimerPtr 0 swap !            ( enemy )
	dup 0 swap SetAnimationFrame        ( enemy )
	dup 
	GetCurrentDirection SetMorphingAnimation
;

: FinishTransformingToDigger ( enemyPtr -- )
	dup GetTimerPtr 0 swap !
	( set new enemy type )
	dup GetEnemyTypePtr EnemyTypeDigger swap !32
	dup                                                     ( enemy enemy )
	GetCharacterTile                                        ( enemy enemy charY charX )
	rot                                                     ( enemy charY charX enemy )
	GetCurrentDestination                                   ( enemy charY charX destY destX )
	SetNewDiggerPath
;

: EnemyUpdate ( enemyptr -- )
	( increment enemy timer with delta time )
	IncrementDeltaTime
	dup                           ( enemy enemy )
	( move enemy along path, returns -1 if moved into a new square )
	' OnReachedPathEnd FollowPath ( enemy hasReachedNewCell )
	-1 = if
		( if moved into new square, reset timer )
		dup GetTimerPtr           ( enemy timerPtr )
		0 swap !
	then
	( set animation )
	dup GetPushing      ( enemy isPushing )
	swap
	dup                 ( isPushing enemy enemy ) 
	dup                 ( isPushing enemy enemy enemy )
	GetCurrentDirection ( isPushing enemy enemy currentDirection )
	2swap               ( enemy currentDirection isPushing enemy )
	rot                 ( enemy isPushing enemy currentDirection )
	rot                 ( enemy enemy currentDirection isPushing )
	swap                ( enemy enemy isPushing currentDirection )
	SetNormalAnimation  ( enemy )
	dup                 ( enemy enemy )
	GetDeltaT           ( enemy enemy deltaT )
	swap
	( update animator )
	UpdateAnimator      ( enemy )
	dup
	( if timer is greater than a certain threshold then become a digger )
	GetTimerPtr @       ( enemy currentTimer )
	EnemyWaitTimeBeforeBecomeDigger > if
		TransformToDigger
	else
		drop
	then
;

: FlashingEnemyUpdate ( enemyPtr -- )
	IncrementDeltaTime
	dup GetTimerPtr @                      ( enemy currentTimer )
	EnemyFlashTime > if
		dup 
		FinishTransformingToDigger
	then
	dup                                   ( enemy enemy )
	dup                                   ( enemy enemy enemy )
	GetCurrentDirection                   ( enemy enemy enemy->CurrentDirection )
	SetMorphingAnimation                  ( enemy )
	GetDeltaT                             ( enemy deltaT )
	swap UpdateAnimator
;