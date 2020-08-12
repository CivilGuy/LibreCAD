/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2020 David J. Mitchell (damitch@civilguy.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>
#include "rs_debug.h"
#include "rs_font.h"
#include "rs_mtext.h"
#include "rs_line.h"

#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

LC_CodeTag::LC_CodeTag(QString& rawtxt, unsigned int* nposn) {
    loadFrom(rawtxt, nposn);
}

LC_CodeTag::~LC_CodeTag() { }

void LC_CodeTag::loadFrom(const QString& rawtxt, unsigned int* nposn) {
    starttag = *nposn;
    text     = rawtxt.mid(starttag);
    endtag   = getFinalEnd();
    isstack  = false;

    text.resize(endtag);

    if (text[0] == '\\') {
        if (QString("ACHQTWX").indexOf(text[1]) != -1) {
            /** For A and C, value is a simple USHORT;
             *  for the others, value is a float, but for H and W could conceivable be either absolute
             *  value, or a relative value ending with an 'x' (or 'X'?) -- not sure how or whether to 
             *  deal with that. For now, figure that H value is absolute and W value is relative to H --
             *  ignore any other text up to final ';'. < FIXME (when?) */
            if ((text[1] == 'A') || (text[1] == 'C')) {
                intVal = this->text.mid(2, (endtag - 2)).toInt(); // TODO: Test this
            } else {
                dblVal = this->text.mid(2, (endtag - 2)).toDouble(); // TODO: Test this
            }
        } else if (QString("Ffp").indexOf(text[1]) != -1) {
            /** The value is font name, family, and opt characteristics for the font -- or it's a
             *  set of paragraph/tab settings.  For now, leave as simple string. */
            strVal = this->text.mid(2, (endtag - 1));
        } else if (text[1] == 'S') {
            isstack = true;
        }
    } else {
        int nStack = text.indexOf(QString("\\S"));
        int nSpace = text.indexOf(QString(" "));
        isstack = (-1 < nStack) && (nStack < nSpace);
    }
    *nposn = endtag;
}

bool LC_CodeTag::isspantag() {
    return isstack || (((text.length() > 1) && (QString("LOKS").indexOf(text[1], Qt::CaseSensitive) != -1))
        || ((text.length() > 0) && (text[0] == '{')));
}

unsigned int LC_CodeTag::getMatchingEnd(const QString& delim, unsigned int start, unsigned short level) {
    /** similar to a previous func getGroups(), but only returns the single index of the close delimiter
     *  at the same 'level' as the first open delimiter - that is, this version is slightly simpler
     *
     *  As written, only works with bracket delims - other spanning codes can't self-nest, so don't
     *  (can't) make use of this method... BUT CHECK (FIXME?) */

    int n0 = this->text.indexOf(delim[0], start), n1 = 0, n2 = 0;
    /** if a second open delimiter is found before the close delimiter (must be at least one close delim)
     *    move one level up and try again.
     *  At some point, getMatchingEnd() will return the position of the close delim that matches the 
     *    open delim that started the call.  Return it to n0. */
        
    while ((n0 != -1) && (n0 < this->text.length())) {
        n1 = this->text.indexOf(delim[0], n0);
        n2 = this->text.indexOf(delim[1], n0);
        
        if ((n1 != -1) && (n1 < n2)) {
            n0 = getMatchingEnd(delim, n1, level + 1) + 1;
        } else {
            break;
        }
    }
    return n2;
}

unsigned int LC_CodeTag::getFinalEnd() {
    /** For 'spanning' and 'stacking' tags, returns the position of the first character of the close tag.
        For 'settings', returns the position just beyond the closing semicolon. */
    int nEnd = 0;

    if (this->isspantag()) {
        if (this->text[0] == '{') {
            nEnd = this->getMatchingEnd("{}"); // recursive, fwiw
        } else if (QString("OLK").indexOf(this->text[1]) != -1) {
            QString closetag = "\\";
            closetag += this->text[1].toLower();
            nEnd = this->text.indexOf(closetag);
        } else { // must be stacking tag
            // FIXME...  implies that there are no other settings tags before end of stacking tag;
            // likely enough, but not dead certain
            nEnd = this->text.indexOf(';');
        }
    } else if ((this->text[0] == '\\') 
            && (QString("olkP").indexOf(this->text[1]) != -1)) {
        // not spanning tag, so must be closing tag or forcebreak
        nEnd += 2;
    } else if ((this->text[0] == '^') && (this->text[1] == 'I')) { // indent tag
        nEnd += 2;
    } else { // regular 'settings' tag
        nEnd = this->text.indexOf(';') + 1;
    }
    return (nEnd == -1) ? text.length() : (nEnd + this->starttag);
}

QPair<LC_MTextTabGroup::LC_MTextTab, double> LC_MTextTabGroup::find(
  LC_MTextTabGroup::LC_MTextTab kind, unsigned short startAt) {
  
    QList<QPair<LC_MTextTabGroup::LC_MTextTab, double>>::iterator
      itrEntry = tablist.begin() + startAt;
    while (itrEntry < tablist.end()) {
        if ((*itrEntry).first == kind) return *itrEntry;
        ++itrEntry;
    }
    return QPair<LC_MTextTabGroup::LC_MTextTab, double>(Invalid, 0.0);
} 

void LC_MTextTabGroup::loadFrom(const QString& strTabs) {
    LC_MTextTab kind = Invalid;
    double      value;
    int n1 = 2, n2 = n1;

    while (strTabs[n2] != ';') {
        switch(strTabs[n1].toLatin1()) {
        case 'i':
            kind    = ItemIndent;
            break;
        case 'l':
            kind    = LeftIndent;
            if (tablist.back().first == ItemIndent) listFormatter = true;
            break;
        case 't':
            kind    = LeftAlign;
            break;
        case 'x':
            kind    = BulletIdx;
            break;
        default:
            // TODO RS_DEBUG->print( << "LC_MTextTabGroup::loadFrom() - unknown code \'" << strTabs[n2].toLatin1() << "\'\n";
            break;
        }
        ++n1;
        n2 = n1;
        while (!((strTabs[n2] == ',') || (strTabs[n2] == ';'))) ++n2;
        value = strTabs.midRef(n1, (n2 - n1)).toDouble();
        n1 = n2 + 1;
        tablist.append(QPair<LC_MTextTab, double>(kind, value));
    }
}

/**
 * Constructor.
 */
RS_MTextData::RS_MTextData(const RS_Vector& _insertionPoint,
			double _height,
			double _width,
			VAlign _valign,
			HAlign _halign,
			MTextDrawingDirection _drawingDirection,
			MTextLineSpacingStyle _lineSpacingStyle,
			double _lineSpacingFactor,
			const QString& _text,
			const QString& _style,
			double _angle,
			RS2::UpdateMode _updateMode):
	insertionPoint(_insertionPoint)
	,height(_height)
	,width(_width)
	,valign(_valign)
	,halign(_halign)
	,drawingDirection(_drawingDirection)
	,lineSpacingStyle(_lineSpacingStyle)
	,lineSpacingFactor(_lineSpacingFactor)
	,text(_text)
	,style(_style)
	,angle(_angle)
	,updateMode(_updateMode)
{
    multiLine    = false;
    textWidthFac = 1.0;
    decoration   = None;
    linebreak    = AtNeed;
    tabs         = nullptr;
    listText     = false;
    vertClear    = 0.0;
}

void RS_MTextData::applyCode(LC_CodeTag& codetag) {
    QChar code = (codetag.text.length() > 1) ? codetag.text[1] : codetag.text[0];
    size_t n0;
    
    switch(code.toLatin1()) {
    case 'A':
        break; // TODO - but still undecided just what to do here
    case 'C':
        break; // TODO - set entity's color, when accesible
    case 'F': case 'f':
        // Codes after a pipe may be present to flag bold, italic, and other values - skipped for now
        n0 = codetag.text.indexOf('|');
        if (n0 == std::string::npos) n0 = codetag.text.indexOf(';');
        this->style = codetag.text.mid(2, n0 - 2);
        break;
    case 'H':
        height = codetag.dblVal;
        break;
    case 'Q': // Oblique factor - skip for now
        break;
    case 'T': // Tracking value (?) - skip for now
        break; // TODO
    case 'W':
        textWidthFac = codetag.dblVal; // but FIXME?
        break;
    case 'X':
        break; // TODO - Dimensioning flag - skip for now
    case 'p':
        tabs.reset(new LC_MTextTabGroup(codetag.text));
        // FIXME later - if codetag.text is __, instead delete (this use of?) tabs
        break;
    case 'P':
        if (linebreak == ForceNow) {
            vertClear += height * STDLINESPACE * lineSpacingFactor;
        } else {
            linebreak = ForceNow;
            vertClear = 0.0;
        }
        break;
    case 'S':
        break; // stacking code handled elsewhere
    case 'L':
        decoration = Underline;
        break;
    case 'O':
        decoration = Overline;
        break;
    case 'K':
        decoration = Strikethru;
        break;
    case '{':
        break; // Handled elsewhere
    case 'l': case 'o': case 'k': case '}':
        break;  // closing codes handled elswhere
    case 'I':
        /* TODO later:
         * Finish review of the whole thing.  Still not fully convinced that it's a full code rather than
         * a special character.  Still isn't figured for use as a regular tab-stop. */
        if (hasListFormat()) listText = true;
        break;
    default: // FIXME
        // RS_DEBUG->print("Warning: unrecognized rich code " << code << "\n";
        break;
    }
}

bool RS_MTextData::hasListFormat() {
    return (tabs != nullptr) && (tabs->tablist.size() > 1) 
      && (tabs->tablist[0].first == LC_MTextTabGroup::ItemIndent) 
      && (tabs->tablist[1].first == LC_MTextTabGroup::LeftIndent);
}

bool RS_MTextData::isListText() {
    return hasListFormat() && (listText == true);
}

void RS_MTextData::resetOneUseVals() {
    linebreak = AtNeed;
    listText  = false;
}

/**
 * Constructor.
 */
RS_MText::RS_MText(RS_EntityContainer* parent,
                 const RS_MTextData& d)
        : RS_EntityContainer(parent), data(d) {

    RS_DEBUG->print("RS_MText constructor - parent is type %d", parent->rtti());

    usedTextHeight = 0.0;
    usedTextWidth = 0.0;
    setText(data.text);
    
    RS_DEBUG->print("End mtext constructor - object is now %s", dump().c_str());

}

RS_Entity* RS_MText::clone() const{
	RS_MText* t = new RS_MText(*this);
	t->setOwner(isOwner());
	t->initId();
	t->detach();
	return t;
}

std::string RS_MText::dump() { // DEBUG - delete from final version
    std::stringstream strOut;

    strOut << std::setbase(10) << std::setiosflags(std::ios_base::fixed) << std::setprecision(1) 
      << "\033[36mMText:\033[0m\nid = " << this->id 
      << ",\nraw text is \'" << data.text.toLatin1().data() << "\'\n"
      << "                     x           y\n"
      << "insertionPoint" << std::setw(10) << data.insertionPoint.x << std::setw(10) << data.insertionPoint.y << "\n"
      << "minV          " << std::setw(10) << minV.x << std::setw(10) << minV.y << "\n"
      << "maxV          " << std::setw(10) << maxV.x << std::setw(10) << maxV.y << "\n"
      << "Contains glyphs? " << (hasGlyphs() ? "Yes" : "No") << "; child count is " << entities.size() << "\n";
    
    return strOut.str();
}

std::string RS_MText::dumpGlyphs() { // DEBUG - delete from final version
    std::stringstream strOut;

    if (hasGlyphs()) {
        RS_Insert *glyph0, *glyphM, *glyphF;
        glyph0 = (RS_Insert *)*(entities.begin());
        glyphM = (RS_Insert *)*(entities.begin() + (entities.size()/2));
        glyphF = (RS_Insert *)*(entities.end() - 1);
        RS_Vector insPt0 = glyph0->getInsertionPoint();
        RS_Vector insPtM = glyph0->getInsertionPoint();
        RS_Vector insPtF = glyph0->getInsertionPoint();

        strOut << std::setbase(10) << std::setiosflags(std::ios_base::fixed) << std::setprecision(1) 
          << "\033[36mChild Glyphs:\033[0m   insert Point   minV     maxV\n"
          << "First \'" << glyph0->getName().toLatin1().data() << "\'  " 
          << insPt0.x << ", " << insPt0.y << "   "
          << glyph0->getMin().x << glyph0->getMin().y << "   "
          << glyph0->getMax().x << glyph0->getMax().y << "\n"

          << "Middle \'" << glyphM->getName().toLatin1().data() << "\'  " 
          << insPtM.x << ", " << insPtM.y  << "   "
          << glyphM->getMin().x << ", " << glyphM->getMin().y << "   "
          << glyphM->getMax().x << ", " << glyphM->getMax().y << "\n"

          << "Last \'" << glyphF->getName().toLatin1().data() << "\'  " 
          << insPtF.x << ", " << insPtF.y  << "   "
          << glyphF->getMin().x << ", " << glyphF->getMin().y << "   "
          << glyphF->getMax().x << ", " << glyphF->getMax().y << "\n\n";
    }
    return strOut.str();
}

/**
 * Gets list of indexes where data.text switches from rich code text to display text
 * and vice versa. A code section may be empty, shown by duplicate index values.
 */
void RS_MText::getSecnStarts(QList<unsigned int>& vecStarts) {

    int n = 0; 
    bool isCodeSecn = true;
    
    vecStarts.clear(); 
    vecStarts.push_back(n); // add another n value when section type shifts

    while (n < data.text.length()) {
        switch(data.text[n].toLatin1()) {
        case '\\':
            if (QString("\\{}~").indexOf(data.text[n + 1]) != -1) {
                // escaped backslash or bracket, or nbsp - treat as text characters
                if (isCodeSecn) { vecStarts.push_back(n); isCodeSecn = false; }
                n++;
            } else {
                if (!isCodeSecn) { // end text Secn, then switch to code section
                    vecStarts.push_back(n); isCodeSecn = true;
                }
                n++;
                if (QString("OLKolk").indexOf(data.text[n]) != -1) {
                    // here, do nothing
                } else if (QString("ACFfHpQTWX").indexOf(data.text[n]) != -1) { 
                    // value is only up to next semicolon
                    n = data.text.indexOf(';', n);
                    // TODO vv - probably want formal exception or assertion here
                    if (n == -1) { // vvv TODO - replace with RS_DEBUG->print calls
                        // TODO RS_DEBUG->print( << "Bad text: code not ended properly.\n"; return;
                    };
                } else if (data.text[n] == 'P') { // 'hard' paragraph break
                    // single character, but needs to be treated as full rich code
                    // here, do nothing // TODO ^^ Last chance to reconsider (until next chance)
                } else if (data.text[n] == 'S') { // stacking code
                    unsigned int n2 = vecStarts.back();
                    if (vecStarts.size() > 2) {
                        while (!data.text[n2].isSpace() && (n2 > 0)) --n2;
                        // the one before vecStarts.back() is the beginning of the display text before the current code secn
                        // if n2 < vecStarts[vecStarts.size() - 2] that entire display text is 'prefix' 
                        // else a code section (empty) is needed before the prefix (unless a space just precedes vecStarts.back())
                        if (n2 >= vecStarts[vecStarts.size() - 2]) {
                            ++n2; // move off the space
                            if (n2 < vecStarts.back()) {
                                vecStarts.insert((vecStarts.end() - 1), n2);
                                vecStarts.insert((vecStarts.end() - 1), n2);
                            }
                        }
                    }
                    n = data.text.indexOf(';', n); // but could \S have spans within itself? FIXME later.
                    if (n == -1) {
                        // TODO RS_DEBUG->print( << "Bad text: stack code not ended properly.\n"; return;
                    } else vecStarts.push_back(n);
                } else {
                    // TODO RS_DEBUG->print( << "Bad text: Unrecognized escape code.\n";
                }
            }
            break;
        case '^':
            if (data.text[n + 1] == 'I') {
                if (!isCodeSecn) { vecStarts.push_back(n); isCodeSecn = true; }
                ++n;
            }
            break;
        case '{': case '}':
            // end text section, then switch to code section
            if (!isCodeSecn) { vecStarts.push_back(n); isCodeSecn = true; } 
            break;
        default:  // it's display text
            if (isCodeSecn) { vecStarts.push_back(n); isCodeSecn = false; } // end it, then switch to text section
            break;
        }   
        n++;
    }
    vecStarts.push_back(n);
}

// TWEAKED signature
unsigned short RS_MText::getSecn(unsigned int nchr, const QList<unsigned int>& vecStarts) {
    unsigned short nsecn = vecStarts.size() - 1;

    while ((nsecn > 0) && (vecStarts[nsecn] > nchr)) --nsecn;
    if ((nsecn > 0) && (vecStarts[nsecn - 1] == vecStarts[nsecn])) --nsecn;

    return nsecn;
}

std::string dumpQList(const QList<unsigned int> &list) {
    std::stringstream ssOut;
    ssOut << "QList {";
    for (int n = 0; n < list.size(); ++n) {
        ssOut << list.at(n);
        if (n < (list.size() - 1)) ssOut << ", ";
    }
    ssOut << "}";
    return ssOut.str();
}

// NEW
bool RS_MText::stackFoundAt(unsigned short nsecn, const QList<unsigned int>& vecSecnStarts) {
    // return true if there's a stack command in this code section, or if there's one in the
    // next code section and there's no space character intervening

    int nStack = data.text.indexOf(QString("\\S"), vecSecnStarts[nsecn]);
    bool isStackFound = false;

    if ((-1 < nStack) && (nsecn < vecSecnStarts.size() - 3)) {
        // else no legit stack is possible 
        if ((unsigned int)nStack < vecSecnStarts[nsecn + 1]) isStackFound = true;
        else if ((vecSecnStarts[nsecn + 2] <= (unsigned int)nStack) 
          && ((unsigned int)nStack < vecSecnStarts[nsecn + 3])) {
            // the stack may have a prefix, but only if there's no space
            // between the prefix and the stack code
            int nSpace = data.text.indexOf(QString(" "), 
                                        vecSecnStarts[nsecn]);
            isStackFound = nSpace > nStack;
        }
    }
    return isStackFound;
}

/**
 * Creates tree structure for top-level MText depending on 'rich codes' encountered in t.
 * Also performs layout() of text, calls setDecorations(), and update()
 */
void RS_MText::setText(const QString& t) {
    if (t.isEmpty()) return;
    
    RS_DEBUG->setLevel(RS_Debug::D_DEBUGGING); // show all debug messages
    RS_DEBUG->print("  Begin setText()\n");
    
    data.text = t;
    
    QList<unsigned int> vecSecnStarts;
    getSecnStarts(vecSecnStarts);  // TODO: TEST -- see getSecnStarts() for separate testing
    
    RS_DEBUG->print("vecSecnStarts is %s", dumpQList(vecSecnStarts).c_str());

    minV.set(data.insertionPoint.x, data.insertionPoint.y - data.height); 
    maxV.set(data.insertionPoint.x, data.insertionPoint.y);

    unsigned int nchr = 0;
    
    if ((2 < vecSecnStarts.size()) && (vecSecnStarts.size() < 5)) { 
        // TODO: TEST -- might test this by ___
        
        // no more than one display section
        
        // replace any '\~' with nbsp - simplest that way
        int idxnbsp = 0;
        idxnbsp = data.text.indexOf("\\~", idxnbsp);
        while (idxnbsp != -1) {
            data.text.replace(idxnbsp, 2, "\xA0");
            idxnbsp = data.text.indexOf("\\~", idxnbsp);
        }

        while (nchr < vecSecnStarts.at(1)) {
            LC_CodeTag ct(data.text, &nchr);
            // TODO RS_DEBUG->print( << "*A* - code tag text is \'" << ct.text.toStdString().c_str() << "\" and nchr is " << nchr << "\n";
            data.applyCode(ct);
        }

        // TODO RS_DEBUG->print( << "MText data is now:\n" << data;

        /* Now add insert for each char */

        RS_Font* font {RS_FONTLIST->requestFont(data.style)};
        if (font == nullptr) {
            return;
        }
        
        double netHeightFac = data.height/9.0, netWidthFac = netHeightFac * data.textWidthFac,
          // note that the font file data is scaled up by 9, per tradition (?)
          netSpaceWidth     = netWidthFac * font->getWordSpacing(),
          netLetterSpace    = netWidthFac * font->getLetterSpacing();

        RS_InsertData insDataTemp(
            QString(""), // name
            RS_Vector(minV),                      // insertionPoint
            RS_Vector(netWidthFac, netHeightFac), // scale factors
            0.0,                                  // angle
            1, 1,                                 // cols, rows
            RS_Vector( 0.0, 0.0),                 // spacing
            font->getLetterList(),                // block source
            RS2::NoUpdate                         // update mode
        ); // only a couple of these properties need to be changed for each char
        
        for (nchr = vecSecnStarts[1]; nchr < vecSecnStarts[2]; ++nchr) {  // See Outline A in djmNotesX1.txt
            const QChar c = data.text[nchr];
            if (c.isSpace()) {
                if ((nchr > vecSecnStarts[1]) && !data.text[nchr - 1].isSpace()) {
                    insDataTemp.insertionPoint.x -= netLetterSpace;
                }
                if ((c == '\x20') || (c == '\xA0')) {
                    insDataTemp.insertionPoint.x += netSpaceWidth;
                }
            } else { // a regular glyph/character
                insDataTemp.name = QString{data.text[nchr]};
                RS_Insert *glyph = new RS_Insert(this, insDataTemp);
                std::stringstream ss;

                this->entities.push_back(glyph);
                glyph->setPen(RS_Pen(RS2::FlagInvalid));
                glyph->setLayer(nullptr);

                if (!font->findLetter(glyph->getName())) {
                    RS_DEBUG->print("  Unrecognized character\n");
                    glyph->setName(QChar(0xfffd)); /* calls update() itself */
                } else glyph->update();

                insDataTemp.insertionPoint.x += 
                  glyph->getSize().x + netLetterSpace;
            }
        }

        if (data.text.at(data.text.size() - 1).isSpace()) 
          insDataTemp.insertionPoint.x -= netLetterSpace;
        
        this->maxV.x = insDataTemp.insertionPoint.x;
        // TODO RS_DEBUG->print( << "End of setText(), single display section\n"; // << *this;

    } else { // more than one display section

        RS_MTextData datanow(this->data);
        RS_MText     *child = nullptr; 
        unsigned short nsecn = 0;
        nchr = 0;
        LC_CodeTag ct;

        while (nsecn < (vecSecnStarts.size() - 2)) {
            nchr = vecSecnStarts[nsecn];
            if (stackFoundAt(nsecn, vecSecnStarts)) {
                ct.loadFrom(data.text, &nchr);
                child = buildStackAssy(datanow, ct);
                nsecn = getSecn(nchr, vecSecnStarts);
            } else {
                while (nchr < vecSecnStarts[nsecn + 1]) {
                    ct.loadFrom(data.text, &nchr); // nchr is updated here
                    if (!ct.isspantag()) {
                        datanow.applyCode(ct);
                    }
                    // else datanow.applyCode(ct);
                }
                if (ct.isspantag()) {
                    datanow.text = data.text.mid(vecSecnStarts[nsecn], 
                      (ct.endtag - vecSecnStarts[nsecn]));
                    nsecn = getSecn(nchr, vecSecnStarts);
                } else {
                    datanow.text = data.text.mid(vecSecnStarts[nsecn], 
                      (vecSecnStarts[nsecn + 2] - vecSecnStarts[nsecn]));
                    nsecn += 2;
                }
                child = new RS_MText(this, datanow);
            }
            entities.push_back(child);
            maxV.x = entities.back()->getMax().x; // um, if glyph holder, might have final space to figure -?
            datanow.insertionPoint.x = maxV.x;    // ^^
        }
        // TODO RS_DEBUG->print( << "End of setText(), multiple display section\n";
    }

    if ((parent == nullptr) || (parent->rtti() != RS2::EntityMText)) {
        calculateBorders();
        layout(data.insertionPoint, data.insertionPoint.x, data.insertionPoint.x + data.width);
        calculateBorders(); // don't want to do this in layout() since that's recursive

        usedTextHeight = maxV.y - minV.y;
        usedTextWidth  = maxV.x - minV.x;
        setDecorations();
    }

    if (data.updateMode == RS2::Update) {
        update();
        RS_DEBUG->print(" Finished mtext update: object is now %s\n\n", dump().c_str());
    }
}

RS_MText* RS_MText::buildStackAssy(const RS_MTextData& data, const LC_CodeTag& codeTag) {
    RS_MTextData datanow(data);
    RS_Vector posnnow(datanow.insertionPoint);
    
    datanow.linebreak = RS_MTextData::Never;
    datanow.text.clear();
    
    RS_MText *pStack(new RS_MText(this, datanow)), *pChild;
    int n = codeTag.text.indexOf("\\S"), n1;
    double baseHeight     = datanow.height;
    unsigned short pieces = 0;  // 1 for prefix, 2 for superscript, 4 for subscript
    QChar   stackstyle    = 0; // '^' for super/sub, '/' for horz divisor, '#' for slanted divisor
    
//? assert(n != -1)
    if (n != 0) { // @A
        pieces += 1;
        datanow.text = codeTag.text.mid(0, n);
        pChild = new RS_MText(this, datanow);
        pStack->appendEntity(pChild);
        datanow.insertionPoint.x = pChild->getMax().x;
        n += 2;
    }
    
    n1 = codeTag.text.indexOf(QRegExp("[^/#]"), n);
    stackstyle = codeTag.text[n1];

//? assert(n1 != std::string::npos)
    if (n1 > n) { // @B
        pieces += 2;

        datanow.text = codeTag.text.mid(n, (n1 - n));
        datanow.insertionPoint.y = posnnow.y + (baseHeight * RAISESUPERSCR);
        // see below - can't hope to properly center this for codeTag.text[n1] == '/'
        // until width of last child is known         

        datanow.height = baseHeight * SUPERSUBFAC;
        pChild = new RS_MText(this, datanow);
        pStack->appendEntity(pChild);
        n = n1 + 1;
    }

    n1 = codeTag.text.indexOf(";", n);
//? assert(n1 != std::string::npos)
    if (n1 > n) { // @C
        pieces += 4;

        datanow.height = baseHeight * SUPERSUBFAC;
        datanow.insertionPoint.y = posnnow.y - (baseHeight * DROPSUBSCRIPT);
        if (stackstyle == '#') {
            datanow.insertionPoint.x = lastEntity()->getMax().x;
        }
        
        datanow.text = codeTag.text.mid(n, (n1 - n));
        pChild = new RS_MText(this, datanow);
        pStack->appendEntity(pChild);
        n = n1 + 1;
    }

    if (pieces > 5) {
        // TODO later - add divider line
    }
    /* FIXME later - if (datanow.text[n1] == '#')
    will want to center the last two childs */
    
    // possible that appendEntity(pChild) above automatically 
    // adjusts borders of pStack - no foolin'? FIXME: TEST -?
        
    return (pStack->count() > 1) ? pStack : nullptr;
}

bool RS_MText::hasGlyphs() {
    return (!entities.empty() && (((RS_Insert *)entities[0])->rtti() == RS2::EntityInsert));
}

RS_Vector RS_MText::layout(const RS_Vector &posnV, double leftMargin, double rightMargin) { 
    /* BEGIN Test Group 5 */
    RS_DEBUG->print("==BEGIN TEST GROUP 5 ==\nleftMargin is %.1f and rightMargin is %.1f\n", leftMargin, rightMargin);
    RS_DEBUG->print("posnV is (%.1f, %.1f)\n", posnV.x, posnV.y);

    RS_Vector delta(posnV - data.insertionPoint);    
    this->move(delta);
    
    RS_DEBUG->print("After initial move, this is %s", this->dump().c_str());
    RS_DEBUG->print(""); // GDB breakpoint
    
    RS_Vector rtrnV(data.insertionPoint);
    
    if (this->wantsLineReturn(leftMargin)) {
        delta.set((leftMargin - data.insertionPoint.x), 
          (-(data.height * STDLINESPACE * data.lineSpacingFactor + data.vertClear)));
        this->move(delta);
        rtrnV = maxV;
        
        RS_DEBUG->print(
          "This wanted line return, so its been moved (%.1f, %.1f). Its rtrnV is now (%.1f, %.1f) and its maxV is (%.1f, %.1f)\n",
          delta.x, delta.y, rtrnV.x, rtrnV.y, maxV.x, maxV.y);
    }
    
    // otherwise:
    if (this->hasGlyphs()) {
        RS_DEBUG->print("This is a glyph holder.\n");
        if (this->data.isListText()) {
            RS_DEBUG->print("  This is list text.\n");
            double localLeftMarg = leftMargin + data.tabs->find(LC_MTextTabGroup::LeftIndent).second;
            delta.set((localLeftMarg - data.insertionPoint.x), 0.0);
            this->move(delta);
            if (maxV.x <= rightMargin) {
                RS_DEBUG->print("    This list text still fits onto one line.\n");
                rtrnV.x = maxV.x;
            } else {
                RS_DEBUG->print("    This list text needs word wrap.\n");
                while (this->wordwrap(localLeftMarg, rightMargin)) {/* continue */};
                rtrnV = ((RS_MText *)entities.back())->getMax();
            }
        } else if (maxV.x <= rightMargin) {
            RS_DEBUG->print("  This regular text still fits onto one line.\n");
            rtrnV.x = maxV.x;
        } else if (this->data.linebreak == data.Never) {
            RS_DEBUG->print("  This regular text still fits onto one line.\n");
            // move to next line if not already at left margin;
            // won't have been done yet and no other help for it
            if (data.insertionPoint.x > leftMargin) {
                RS_Vector delta((leftMargin - data.insertionPoint.x), 
                  (-data.height * STDLINESPACE * data.lineSpacingFactor));
                this->move(delta);
                rtrnV = maxV;
            }
        } else {
            RS_DEBUG->print("  This regular text needs word wrap.\n");
            while (wordwrap(leftMargin, rightMargin)) {/* continue */}
            rtrnV = ((RS_MText *)entities.back())->getMax();
        }
    } else {
        RS_DEBUG->print("This mtext is NOT a glyph holder, so layout its children.\n");
        for (auto &child : entities) {
            // ^^ Huh.  Might have just caught a decently bad bug - had *not*
            // been calling child as a ref, so would have been a copy, not the 'real thing'
            rtrnV = ((RS_MText *)child)->layout(rtrnV, leftMargin, rightMargin);
        }
    }

    usedTextWidth  = getSize().x;
    usedTextHeight = getSize().y;
    
    RS_DEBUG->print(
      "Finished layout() for %s  rtrnV is now (%f, %f); usedTextWidth is now %f, and usedTextHeight is now %f\n",
      this->dump().c_str(), rtrnV.x, rtrnV.y, usedTextWidth, usedTextHeight);
    
    return rtrnV;
}

bool RS_MText::wordwrap(double leftMarg, double rightMarg) {
    bool iswrap = false;
    RS_DEBUG->print("Begin wordwrap(). hasGlyphs is %d\n", this->hasGlyphs());

    if (this->hasGlyphs()) {
        RS_DEBUG->print("Before word split, representative glyphs are:%s", this->dumpGlyphs().c_str());
    
        QList<RS_Entity *>::iterator itrGlyph = entities.end() - 1;
        QString::iterator itrTxt = data.text.end() - 1;
        
        /* if method here is to start at the right and work 'backwards', *A*,
         * can only test for condition where last char of a word *just* fits by 
         * looking at the text just following it to see if it's a space */
        
        /* if, instead, method is to start at the left and work forwards, *B*,
         * first need to sync itrTxt and itrGlyph to first char, then stop
         * when some char does not fit, then work backwards to space.  Still, if no
         * space found, could still be the case that a single char of chunk
         * would fit; still need to see if text following it is a space */
         
        /* Try method *A* */
        while ((itrGlyph > entities.begin()) && 
          ((((RS_Insert *)(*itrGlyph))->getMax().x > rightMarg))) {
            --itrTxt; while((*itrTxt).isSpace()) --itrTxt;
            --itrGlyph;
        }
        //? assert (*itrTxt == ((RS_Insert *)(*itrGlyph))->getName()[0]);
        // (should be able to run these as a test)
        
        if (itrGlyph == (entities.end() - 1)) { // entire line fits without wordwrap
            return false;
        } else if ((*(itrTxt + 1)).isSpace()) {
            ++itrGlyph; itrTxt += 2;  // sync up to char following space
        } else {
            while ((itrGlyph > entities.begin()) && !(*itrTxt).isSpace()) {
                --itrTxt; --itrGlyph;
            }
            if ((*itrTxt).isSpace()) {
                // itrGlyph has overshot it, maybe to begin() entity even
                ++itrGlyph; ++itrTxt; // place where break needs to take place
            } else {
                // itrGlyph has moved to begin() without finding any space
                RS_Vector delta(leftMarg - data.insertionPoint.x,
                  (-data.height * STDLINESPACE * data.lineSpacingFactor));
                this->move(delta);
                return true;
            }
        }
        // wordbreak is needed; itrGlyph and itrTxt should now be in proper place
        data.multiLine = false;
        QList<RS_Entity *> tempents;
        RS_MTextData datanow(this->data);
        datanow.text.clear();
        datanow.multiLine = true;
        
        tempents.push_back(new RS_MText(this, datanow));
        tempents.push_back(new RS_MText(this, datanow));
                
        ((RS_MText *)(tempents.front()))->resetFrom(entities.begin(), itrGlyph, 
          data.text.left(itrTxt - data.text.begin()));
        RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "After split, tempents.front() is %s",
           ((RS_MText *)tempents.front())->dump().c_str());  
          
        ((RS_MText *)(tempents.last()))->resetFrom(itrGlyph, entities.end(), 
          data.text.right(data.text.end() - itrTxt));
        RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "tempents.back() is %s", 
          ((RS_MText *)tempents.last())->dump().c_str());

        RS_Vector delta(leftMarg - ((RS_MText *)tempents.last())->getInsertionPoint().x,
          (-data.height * STDLINESPACE * data.lineSpacingFactor));
          
        ((RS_MText *)tempents.last())->move(delta);
        
        this->entities.clear();
        
        //XXX this->clear(); // this removes all the glyph insert pointers *and deletes them*
        // (including the ones just copied to tempents)
        
        this->entities.swap(tempents);
        iswrap = true;
        
    } else {
        RS_MText *pLastChild = (RS_MText *)entities.takeLast();
        while (pLastChild->wordwrap(leftMarg, rightMarg)) {
            entities.append(pLastChild->entities);
            pLastChild->entities.clear();
            delete pLastChild;

            pLastChild = (RS_MText *)entities.takeLast();
        }
        iswrap = false; // since childs are done with it
    }
    return iswrap;
}

/**
 * Needed during word wrap to change out a child object's text and
 * glyphs without otherwise interfering with child's properties, except
 * as shown */
void RS_MText::resetFrom(
      const QList<RS_Entity *>::iterator& startEnts,
      const QList<RS_Entity *>::iterator& endEnts,
      const QString& _text) {
    QList<RS_Entity *>::iterator itrEnt = startEnts;

    data.text = _text;
    while (itrEnt < endEnts) {
        entities.append(*itrEnt);
        ++itrEnt;
    }
    data.insertionPoint.x = ((RS_Insert *)(*startEnts))->getInsertionPoint().x; // .y is unchanged 
    // well enough, since data is otherwise copied faithfully
    
    maxV.set(((RS_Insert *)entities.back())->getMax().x, data.insertionPoint.y);
    // ^^ Corrected from getInsertionPoint() to getMax()
    
    minV.set(data.insertionPoint.x, ((RS_Insert *)entities.front())->getInsertionPoint().y);
    // not strictly accurate because of glyph descenders, but good enough
}

bool RS_MText::wantsLineReturn(double leftMargin) {
    // only returns true if text is right of leftMargin
    RS_DEBUG->print("Wants line return? %f > %f and ((%d == %d) or (%d and %d))",
      data.insertionPoint.x,  leftMargin, data.linebreak, data.ForceNow, data.hasListFormat(), !data.isListText());
    RS_DEBUG->print(""); // GDB breakpoint
    return (data.insertionPoint.x > leftMargin)
      && ((data.linebreak == data.ForceNow) 
        || (data.hasListFormat() && !data.isListText()));
}

void RS_MText::setDecorations() { // called initially by top MText, then recursive
    if ((data.decoration != RS_MTextData::MTextDecor::None) && !data.multiLine) { // for now, only supports underline
        double y_ul = minV.y - data.height * DROPUNDERLINE;
        RS_LineData lineData(RS_Vector(minV.x, y_ul), RS_Vector(maxV.x, y_ul));

        entities.push_back(new RS_Line(this, lineData));
    } else if (!this->hasGlyphs()) {
        for (RS_Entity* child: entities) {
            ((RS_MText *)child)->setDecorations();
        }
    }
}

void RS_MText::forcedCalculateBorders() {
    RS_EntityContainer::forcedCalculateBorders();

    if (this->hasGlyphs() && data.text.at(data.text.size() - 1).isSpace()) {
        RS_Font* font {RS_FONTLIST->requestFont(data.style)};
    
        if (font == nullptr) {
            RS_DEBUG->print("font not found. Arrgh!\n");
            return;
        }
        maxV.x += (data.height/9.0) * data.textWidthFac * font->getWordSpacing();
    }
}

/**
 * Gets the alignment as an int.
 *
 * @return  1: top left ... 9: bottom right
 */
int RS_MText::getAlignment() {
    if (data.valign==RS_MTextData::VATop) {
        if (data.halign==RS_MTextData::HALeft) {
            return 1;
        } else if (data.halign==RS_MTextData::HACenter) {
            return 2;
        } else if (data.halign==RS_MTextData::HARight) {
            return 3;
        }
    } else if (data.valign==RS_MTextData::VAMiddle) {
        if (data.halign==RS_MTextData::HALeft) {
            return 4;
        } else if (data.halign==RS_MTextData::HACenter) {
            return 5;
        } else if (data.halign==RS_MTextData::HARight) {
            return 6;
        }
    } else if (data.valign==RS_MTextData::VABottom) {
        if (data.halign==RS_MTextData::HALeft) {
            return 7;
        } else if (data.halign==RS_MTextData::HACenter) {
            return 8;
        } else if (data.halign==RS_MTextData::HARight) {
            return 9;
        }
    }

    return 1;
}

/**
 * Sets the alignment from an int.
 *
 * @param a 1: top left ... 9: bottom right
 */
void RS_MText::setAlignment(int a) {
    switch (a%3) {
    default:
    case 1:
        data.halign = RS_MTextData::HALeft;
        break;
    case 2:
        data.halign = RS_MTextData::HACenter;
        break;
    case 0:
        data.halign = RS_MTextData::HARight;
        break;
    }

    switch ((int)ceil(a/3.0)) {
    default:
    case 1:
        data.valign = RS_MTextData::VATop;
        break;
    case 2:
        data.valign = RS_MTextData::VAMiddle;
        break;
    case 3:
        data.valign = RS_MTextData::VABottom;
        break;
    }

}

/**
 * @return Number of lines in this text entity.
 */
/*XXX
int RS_MText::getNumberOfLines() {
    int c=1;

    for (int i=0; i<(int)data.text.length(); ++i) {
        if (data.text.at(i).unicode()==0x0A) {
            c++;
        }
    }
    return c;
} */

/** FIXME - rewrite
 * Updates the Inserts (letters) of this text. Called when the
 * text or its data, position, alignment, .. changes.
 */
void RS_MText::update() // FIXME
{
    RS_DEBUG->print("RS_MText::update");

    if (isUndone()) {
        return;
    }
    for (auto e: entities) {
        e->update();
    }

    forcedCalculateBorders();

    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "  Finished RS_MText::update() - this is now %s", dump().c_str());

    RS_DEBUG->print("RS_MText::update: OK");
}

RS_Vector RS_MText::getNearestEndpoint(const RS_Vector& coord, double* dist)const {
    if (dist) {
        *dist = data.insertionPoint.distanceTo(coord);
    }
    return data.insertionPoint;
}

RS_VectorSolutions RS_MText::getRefPoints() const{
		return RS_VectorSolutions({data.insertionPoint});
}

void RS_MText::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    data.insertionPoint.move(offset);
//    update();
}

void RS_MText::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    RS_EntityContainer::rotate(center, angleVector);
    data.insertionPoint.rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle+angle);
//    update();
}

void RS_MText::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    data.insertionPoint.rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle+angleVector.angle());
//    update();
}

void RS_MText::scale(const RS_Vector& center, const RS_Vector& factor) {
    data.insertionPoint.scale(center, factor);
    data.width*=factor.x;
    data.height*=factor.x;
    update();
}

void RS_MText::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.insertionPoint.mirror(axisPoint1, axisPoint2);
    //double ang = axisPoint1.angleTo(axisPoint2);
    bool readable = RS_Math::isAngleReadable(data.angle);

	RS_Vector vec = RS_Vector::polar(1.0, data.angle);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    data.angle = vec.angle();

    bool corr;
    data.angle = RS_Math::makeAngleReadable(data.angle, readable, &corr);

    if (corr) {
        if (data.halign==RS_MTextData::HALeft) {
            data.halign=RS_MTextData::HARight;
        } else if (data.halign==RS_MTextData::HARight) {
            data.halign=RS_MTextData::HALeft;
        }
    } else {
        if (data.valign==RS_MTextData::VATop) {
            data.valign=RS_MTextData::VABottom;
        } else if (data.valign==RS_MTextData::VABottom) {
            data.valign=RS_MTextData::VATop;
        }
    }
    update();
}



bool RS_MText::hasEndpointsWithinWindow(const RS_Vector& /*v1*/, const RS_Vector& /*v2*/) {
    return false;
}



/**
 * Implementations must stretch the given range of the entity
 * by the given offset.
 */
void RS_MText::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {

    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    }
}


void RS_MText::draw(RS_Painter* painter, RS_GraphicView* view, double& /*patternOffset*/)
{
    if (!(painter && view)) {
        return;
    }

    if (!view->isPrintPreview() && !view->isPrinting())
    {
        if (view->isPanning() || view->toGuiDY(getHeight()) < 4)
        {
            painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
            return;
        }
    }

    foreach (auto e, entities)
    {
        view->drawEntity(painter, e);
    }
}
