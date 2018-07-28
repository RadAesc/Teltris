#include "Tetris1P.h"
#include <algorithm>
#include "SpriteEffect.h"
#include "Random.h"

Tetris1P::Tetris1P( uint seed,Graphics& gfx )
	:
	gfx( gfx ),
	rngEng( seed )
{
	bag.reserve( bagSize );
	for( int i = 0; i < bagSize; ++i )
	{
		bag.emplace_back( Tetreon::Type::Fake );
	}
	RandomizeBag();

	keys.insert( { VK_LEFT,false } );
	keys.insert( { VK_RIGHT,false } );
	keys.insert( { VK_DOWN,false } );
	keys.insert( { VK_UP,false } );
	keys.insert( { int( 'C' ),false } );
	keys.insert( { VK_SPACE,false } );

	for( int i = 0; i < nNextPieces; ++i )
	{
		// nextPieces[i] = Tetreon::GetRandPiece();
		nextPieces[i] = Tetreon::GetPiece( GetNextBagItem() );
	}

	ResetPlayer();

	const Surface base = "Images/Tetreons.bmp";
	pieceIcons.emplace_back( Surface( base,
		RectI( { 8 * 3,8 * 1 },8,8 ) )
		.GetExpanded( size,size ) );
	for( int y = 0; y < 2; ++y )
	{
		for( int x = 0; x < 4; ++x )
		{
			pieceIcons.emplace_back( Surface( base,
				RectI( { 8 * x,8 * y },8,8 ) )
				.GetExpanded( size,size ) );
		}
	}
}

void Tetris1P::Update( const float dt,const Keyboard& kbd )
{
	if( kbd.KeyIsPressed( VK_DOWN ) )
	{
		downTimer.Update( dt );
	}
	else
	{
		downTimer.Reset();
	}
	// 
	if( kbd.KeyIsPressed( VK_LEFT ) && !keys[VK_LEFT] )
	{
		keys[VK_LEFT] = true;
		// piece.GetPos().x -= size;
		// CheckForCollision( { 1,0 },false );
		MovePlayer( -1 );
	}
	else if( kbd.KeyIsPressed( VK_RIGHT ) && !keys[VK_RIGHT] )
	{
		keys[VK_RIGHT] = true;
		// piece.GetPos().x += size;
		// CheckForCollision( { -1,0 },false );
		MovePlayer( 1 );
	}
	else if( kbd.KeyIsPressed( VK_DOWN ) &&
		( !keys[VK_DOWN] || downTimer.IsDone() ) )
	{
		downTimer.Reset();
		Drop();
		dropTimer.Reset();
		keys[VK_DOWN] = true;
	}
	else if( kbd.KeyIsPressed( VK_UP ) && !keys[VK_UP] )
	{
		RotatePlayer( 1 );
		keys[VK_UP] = true;
	}
	else if( kbd.KeyIsPressed( int( 'C' ) ) &&
		!keys[int( 'C' )] )
	{
		SwapStored();
		keys[int( 'C' )] = true;
	}
	else if( kbd.KeyIsPressed( VK_SPACE ) &&
		!keys[VK_SPACE] )
	{
		// This is kinda cool, I think it works but
		//  will run 1 too many times.  That's ok.
		while( !Drop() );
		dropTimer.Reset();
		downTimer.Reset();
	}

	keys[VK_UP] = kbd.KeyIsPressed( VK_UP );
	keys[VK_DOWN] = kbd.KeyIsPressed( VK_DOWN );
	keys[VK_LEFT] = kbd.KeyIsPressed( VK_LEFT );
	keys[VK_RIGHT] = kbd.KeyIsPressed( VK_RIGHT );
	keys[int( 'C' )] = kbd.KeyIsPressed( int( 'C' ) );
	keys[VK_SPACE] = kbd.KeyIsPressed( VK_SPACE );

	dropTimer.Update( dt );
	if( dropTimer.IsDone() )
	{
		Drop();
	}
}

void Tetris1P::Draw( Graphics& gfx ) const
{
	static constexpr Vei2 start = Vei2( 150,50 );
	static constexpr Vei2 storedStart = start +
		Vei2{ Arena::width * size,0 } +Vei2( 5 );
	static constexpr Vei2 nextPieceAdd = Vei2( 0,5 );

	arena.Draw( start,gfx );

	DrawMatrix( arena.GetMat(),
		start,Arena::dim );

	DrawPiece( piece,start );
	DrawGhostPiece( start );

	DrawMatrix( storedPiece.GetMat(),
		storedStart,storedPiece.GetDim() );

	// for( const auto& t : nextPieces )
	for( int i = 0; i < nNextPieces; ++i )
	{
		const auto& next = nextPieces[i];
		// Use ( i + 1 ) instead of i to make room
		//  for stored piece.
		const auto myPos = storedStart +
			nextPieceAdd * ( i + 1 ) +
			Vei2( 0,Tetreon::dimL * size * ( i + 1 ) );

		DrawMatrix( next.GetMat(),myPos,
			next.GetDim() );
	}
}

void Tetris1P::DrawPiece( const Tetreon& piece,
	const Vei2& pos ) const
{
	const auto& mat = piece.GetMat();
	const auto& offset = piece.GetPos() + pos;

	DrawMatrix( mat,offset,
		{ piece.GetDim(),piece.GetDim() } );
}

void Tetris1P::DrawGhostPiece( const Vei2& offset ) const
{
	const auto& mat = ghostPiece.GetMat();
	const auto dim = Vei2( ghostPiece.GetDim() );

	const auto xAdd = offset.x + ghostPiece.GetPos().x;
	const auto yAdd = offset.y + ghostPiece.GetPos().y;
	for( int y = 0; y < dim.y; ++y )
	{
		for( int x = 0; x < dim.x; ++x )
		{
			const auto spot = y * dim.x + x;
			if( spot < int( mat.size() ) &&
				spot >= 0 &&
				mat[spot] != 0 )
			{
				const auto xPos = size * x + xAdd;
				const auto yPos = size * y + yAdd;

				gfx.DrawSprite( xPos,yPos,
					ghostIcon,
					SpriteEffect
					::Chroma{ Colors::Magenta } );
			}
		}
	}
}

void Tetris1P::DrawMatrix( const std::vector<uint>& mat,
	const Vei2& offset,const Vei2& dim ) const
{
	for( int y = 0; y < dim.y; ++y )
	{
		for( int x = 0; x < dim.x; ++x )
		{
			const auto spot = y * dim.x + x;
			if( spot < int( mat.size() ) &&
				spot >= 0 &&
				mat[spot] != 0 )
			{
				// TODO: Replace with images someday.
				// gfx.DrawRect( size * x + offset.x,
				// 	size * y + offset.y,
				// 	size,size,
				// 	Tetreon::tetCols[mat[spot]] );
				gfx.DrawSprite( size * x + offset.x,
					size * y + offset.y,
					pieceIcons[mat[spot]],
					SpriteEffect::Copy{} );
			}
		}
	}
}

bool Tetris1P::Drop()
{
	dropTimer.Reset();
	piece.GetPos().y += size;

	// CheckForCollision( { 0,-1 },true );
	if( arena.Collide( piece ) )
	{
		piece.GetPos().y -= size;
		arena.Merge( piece );
		ResetPlayer();
		score += arena.Sweep( curLevel );
		// UpdateGhost();
		return( true );
	}
	else
	{
		// UpdateGhost();
		return( false );
	}
}

void Tetris1P::UpdateGhost()
{
	ghostPiece = piece;

	bool done = false;
	while( !done )
	{
		ghostPiece.GetPos().y += size;

		if( arena.Collide( ghostPiece ) )
		{
			ghostPiece.GetPos().y -= size;
			done = true;
		}
	}
}

void Tetris1P::MovePlayer( int amount )
{
	Vei2& pos = piece.GetPos();
	// const auto spot = ( pos.x + amount * size ) / size;
	// if( spot < 0 || spot > Arena::width )
	// {
	// 	return;
	// }
	pos.x += amount * size;
	if( arena.Collide( piece ) )
	{
		pos.x -= amount * size;
	}

	UpdateGhost();
}

void Tetris1P::RotatePlayer( int dir )
{
	const auto origXPos = piece.GetPos().x;
	piece = Tetreon::Rotate( piece,dir );
	int offset = 1;
	while( arena.Collide( piece ) )
	{
		piece.GetPos().x += offset * size;

		offset = -( offset + ( offset > 0 ? 1 : -1 ) );

		if( offset > piece.GetDim() * 2/*0*/ )
		{
			piece = Tetreon::Rotate( piece,-dir );
			piece.GetPos().x = origXPos;
			break;
		}
	}

	UpdateGhost();
}

void Tetris1P::ResetPlayer()
{// piece = Tetreon::GetRandPiece();
	piece = nextPieces[0];
	ShiftNextPieces();
	// nextPieces[nNextPieces - 1] = Tetreon::GetRandPiece();
	nextPieces[nNextPieces - 1] = Tetreon::GetPiece( GetNextBagItem() );

	ResetPlayerPos();
}

void Tetris1P::ResetPlayerPos()
{
	piece.GetPos().x = Arena::width / 2 * size;
	piece.GetPos().y = 0;

	dropTimer.Reset();

	if( arena.Collide( piece ) )
	{
		arena.Clear();
		ResetPlayer();
	}

	UpdateGhost();
}

void Tetris1P::ShiftNextPieces()
{
	// for( int i = nNextPieces - 2; i >= 0; --i )
	// {
	// 	nextPieces[i] = nextPieces[i + 1];
	// }
	for( int i = 0; i < nNextPieces - 1; ++i )
	{
		nextPieces[i] = nextPieces[i + 1];
	}
}

void Tetris1P::SwapStored()
{
	if( storedPiece.GetType() != Tetreon::Type::Fake )
	{
		std::swap( piece,storedPiece );
		ResetPlayerPos(); // Doesn't change piece!
	}
	else
	{
		storedPiece = piece;
		ResetPlayer(); // Changes piece!
	}
}

void Tetris1P::RandomizeBag()
{
	// Wow this is gross but I always mess up for loops.
	bag[0] = Tetreon::Type::T;
	bag[1] = Tetreon::Type::O;
	bag[2] = Tetreon::Type::L;
	bag[3] = Tetreon::Type::J;
	bag[4] = Tetreon::Type::I;
	bag[5] = Tetreon::Type::S;
	bag[6] = Tetreon::Type::Z;
	std::shuffle( bag.begin(),bag.end(),rngEng );
}

Tetreon::Type Tetris1P::GetNextBagItem()
{
	auto temp = bag[curBagSpot];
	bag[curBagSpot] = Tetreon::Type::Fake;
	++curBagSpot;
	if( curBagSpot >= bagSize )
	{
		curBagSpot = 0;
		RandomizeBag();
	}
	return( temp );
}