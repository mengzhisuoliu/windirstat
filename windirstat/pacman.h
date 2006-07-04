// pacman.h - Declaration of CPacman
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2005 Bernhard Seifert
// Copyright (C) 2004-2006 Oliver Schneider (assarbad.net)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author(s): - bseifert -> bseifert@users.sourceforge.net, bseifert@daccord.net
//            - assarbad -> http://assarbad.net/en/contact
//
// $Header$

#pragma once

//
// CPacman. Pacman animation.
//
class CPacman
{
public:
	CPacman();
	void SetBackgroundColor(COLORREF color);
	void SetSpeed(double speed);
	void Reset();
	void Start(bool start);
	bool Drive(ULONGLONG readJobs);	// return: true -> should be redrawn.
	void Draw(CDC *pdc, const CRect& rc);

private:
	void UpdatePosition(double& position, bool& up, double diff);
	COLORREF CalculateColor();

	bool m_isWindows9x;		// True if we are running on Windows9x/me, false for NT and higher.
	COLORREF m_bgcolor;		// Background color
	double m_speed;			// Speed in full width / ms
	bool m_moving;			// Whether pacman is moving
	double m_readJobs;		// # of read jobs determines our color 
	bool m_toTheRight;		// moving right
	double m_position;		// 0...1
	bool m_mouthOpening;	// Mouth is opening
	double m_aperture;		// 0...1
	DWORD m_lastUpdate;		// TickCount
};

// $Log$
// Revision 1.7  2006/07/04 23:37:40  assarbad
// - Added my email address in the header, adjusted "Author" -> "Author(s)"
// - Added CVS Log keyword to those files not having it
// - Added the files which I forgot during last commit
//
// Revision 1.6  2006/07/04 22:49:21  assarbad
// - Replaced CVS keyword "Date" by "Header" in the file headers
//
// Revision 1.5  2006/07/04 20:45:23  assarbad
// - See changelog for the changes of todays previous check-ins as well as this one!
//
// Revision 1.4  2004/11/05 16:53:07  assarbad
// Added Date and History tag where appropriate.
//
