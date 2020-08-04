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


#ifndef RS_MTEXT_H
#define RS_MTEXT_H

#include <memory>
#include "rs_entitycontainer.h"

#define STDLINESPACE 1.666667
// Obscure note in Dxf ref says that 5/3 is the standard, but may be modified by code 44 value.
#define SUPERSUBFAC   0.58
// Multiplier for full text height
#define DROPSUBSCRIPT 0.33
#define RAISESUPERSCR 0.33
// Above 3 values taken from Wikipedia 
#define DROPUNDERLINE 0.33
// Same as underscore char in standard.lff 

/**
 * Helper class for working with 'rich codes' embedded in MText
 */
struct LC_CodeTag {

    LC_CodeTag() = default;
    LC_CodeTag(QString& rawtxt, unsigned int* nposn);
    ~LC_CodeTag();
     
    void loadFrom(const QString& rawtxt, unsigned int* nposn);
    
    /** for spanning codes, this->text is the simple command itself
     *  for settings codes, this->text is the entire command and value, including the last semicolon. */
    QString      text;
    bool         isstack;    
    double       dblVal;
    int          intVal;
    QString      strVal;
    unsigned int starttag, endtag;
    
    unsigned int end() { return endtag; } 
    bool isspantag();
    bool isStack() { return isstack; }
    unsigned int getMatchingEnd(const QString& delim, unsigned int start = 0, unsigned short level = 0);
    unsigned int getFinalEnd();
};

/**
 * Helper class for working with tab stops and list formatting in MText
 */
struct LC_MTextTabGroup {
    LC_MTextTabGroup() = default;
    LC_MTextTabGroup(QString& strTabs) { loadFrom(strTabs); }
    ~LC_MTextTabGroup() = default;

    enum LC_MTextTab {
        Invalid,
        LeftAlign,
        CenterAlign,
        RightAlign,
        DP_Align,
        ItemIndent,
        LeftIndent,
        BulletIdx
    };
    QPair<LC_MTextTabGroup::LC_MTextTab, double> 
       find(LC_MTextTab kind, unsigned short startAt = 0);
    bool listFormatter = false;
    bool isListFormat() { return listFormatter; }
    QList<QPair<LC_MTextTab, double>> tablist;
    void clear() { tablist.clear(); }
    void loadFrom(const QString& strTabs);
};

/**
 * Holds the data that defines a text entity.
 */
struct RS_MTextData {
	/**
	 * Mode for line breaks
	 */
    enum MTextBreakMode {
        AtNeed,
        Never,
        ForceNow
    };
    
	/**
	 * Text 'decorations'
	 */
    enum MTextDecor {
        None,
        Underline,
        Overline,
        Strikethru
    };
	/**
	 * Vertical alignments.
	 */
	enum VAlign {
		VATop,      /**< Top. */
		VAMiddle,   /**< Middle */
		VABottom    /**< Bottom */
	};

	/**
	 * Horizontal alignments.
	 */
	enum HAlign {
		HALeft,     /**< Left */
		HACenter,   /**< Centered */
		HARight     /**< Right */
	};

	/**
	 * MText drawing direction.
	 */
	enum MTextDrawingDirection {
		LeftToRight,     /**< Left to right */
		TopToBottom,     /**< Top to bottom */
		ByStyle          /**< Inherited from associated text style */
	};

	/**
	 * Line spacing style for MTexts.
	 */
	enum MTextLineSpacingStyle {
		AtLeast,        /**< Taller characters will override */
		Exact           /**< Taller characters will not override */
	};

	/**
	 * Default constructor. Leaves the data object uninitialized.
	 */
	RS_MTextData() = default;

	/**
	 * Constructor with initialisation.
	 *
	 * @param insertionPoint Insertion point
	 * @param height Nominal (initial) text height
	 * @param width Reference rectangle width
	 * @param valign Vertical alignment
	 * @param halign Horizontal alignment
	 * @param drawingDirection Drawing direction
	 * @param lineSpacingStyle Line spacing style
	 * @param lineSpacingFactor Line spacing factor
	 * @param text Text string
	 * @param style Text style name
	 * @param angle Rotation angle
	 * @param updateMode RS2::Update will update the text entity instantly
	 *    RS2::NoUpdate will not update the entity. You can update
	 *    it later manually using the update() method. This is
	 *    often the case since you might want to adjust attributes
	 *    after creating a text entity.
	 */
	RS_MTextData(const RS_Vector& insertionPoint,
				 double height,
				 double width,
				 VAlign valign,
				 HAlign halign,
				 MTextDrawingDirection drawingDirection,
				 MTextLineSpacingStyle lineSpacingStyle,
				 double lineSpacingFactor,
				 const QString& text,
				 const QString& style,
				 double angle,
				 RS2::UpdateMode updateMode = RS2::Update);

	/** Insertion point */
	RS_Vector insertionPoint;
	/** Nominal (initial) text height */
	double height;
	/** Reference rectangle width */
	double width;
	/** Vertical alignment */
	VAlign valign;
	/** Horizontal alignment */
	HAlign halign;
	/** Drawing direction */
	MTextDrawingDirection drawingDirection;
	/** Line spacing style */
	MTextLineSpacingStyle lineSpacingStyle;
	/** Line spacing factor */
	double lineSpacingFactor;
	/** Text string */
	QString text;
	/** Text style name */
	QString style;
	/** Rotation angle */
	double angle;
	/** Update mode */
	RS2::UpdateMode updateMode;

    /** whether mtext has children on more than one line - used for figuring where in the
     *  tree structure to add decorations */
    bool       multiLine;
	/** text width factor set by rich code*/ 
    double     textWidthFac;

    /** underline, overline, strikethrough set by rich code*/
    MTextDecor       decoration;
	/** line break mode set by rich code*/
    MTextBreakMode   linebreak;
	/** tab settings set by rich code, shared with children and subsequent
	 * mtext siblings until reset for one of them */
    std::shared_ptr<LC_MTextTabGroup> tabs;
	/** whether the display text is the entry for a list item */
    bool             listText;
	/** the extra distance to be set between lines; produced 
     * by extra \P codes */
    double           vertClear;

	/** convert rich codes into data settings */
    void applyCode(LC_CodeTag& codetag);
	/** @return whether this data has the tab settings for a text list */
    bool hasListFormat();
	/** @return whether this should be laid out as list item text */
    bool isListText();
    
	/** reset the one-use values of forced line break and listText */
    void resetOneUseVals();
};

std::ostream& operator << (std::ostream& os, const RS_MTextData& td);

/**
 * Class for a text entity.
 *
 * @author Andrew Mustun, David Mitchell
 */
class RS_MText : public RS_EntityContainer {
public:
    RS_MText(RS_EntityContainer* parent,
            const RS_MTextData& d);
	virtual ~RS_MText() = default;

    virtual RS_Entity* clone() const override;
    
    std::string dump(); //XXX DEBUG

    /**	@return RS2::EntityText */
    virtual RS2::EntityType rtti() const override{
        return RS2::EntityMText;
    }

    /** @return Copy of data that defines the text. */
    RS_MTextData getData() const {
        return data;
    }
    
    void forcedCalculateBorders();
    
    void update() override;

    RS_Vector getInsertionPoint() {
        return data.insertionPoint;
    }
    double getHeight() {
        return data.height;
    }
    void setHeight(double h) {
        data.height = h;
    }
    double getWidth() {
        return data.width;
    }
    void setAlignment(int a);
    int getAlignment();
    RS_MTextData::VAlign getVAlign() {
        return data.valign;
    }
    void setVAlign(RS_MTextData::VAlign va) {
        data.valign = va;
    }
    RS_MTextData::HAlign getHAlign() {
        return data.halign;
    }
    void setHAlign(RS_MTextData::HAlign ha) {
        data.halign = ha;
    }
    RS_MTextData::MTextDrawingDirection getDrawingDirection() {
        return data.drawingDirection;
    }
    RS_MTextData::MTextLineSpacingStyle getLineSpacingStyle() {
        return data.lineSpacingStyle;
    }
    void setLineSpacingFactor(double f) {
        data.lineSpacingFactor = f;
    }
    double getLineSpacingFactor() {
        return data.lineSpacingFactor;
    }
        
    void setText(const QString& t);
    QString getText() {
        return data.text;
    }
    void setStyle(const QString& s) {
        data.style = s;
    }
    QString getStyle() {
        return data.style;
    }
    void setAngle(double a) {
        data.angle = a;
    }
    double getAngle() {
        return data.angle;
    }
    double getUsedTextWidth() {
        return usedTextWidth;
    }
    double getUsedTextHeight() {
        return usedTextHeight;
    }

    /**
     * @return The insertion point as endpoint.
     */
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = NULL)const override;
    virtual RS_VectorSolutions getRefPoints() const override;

    virtual void move(const RS_Vector& offset) override;
    virtual void rotate(const RS_Vector& center, const double& angle) override;
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    virtual void scale(const RS_Vector& center, const RS_Vector& factor) override;
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    virtual bool hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2) override;
    virtual void stretch(const RS_Vector& firstCorner,
                         const RS_Vector& secondCorner,
                         const RS_Vector& offset) override;

    friend std::ostream& operator << (std::ostream& os, const RS_Text& p);

    void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;

protected:
    RS_MTextData data;

    /**
     * Text width used by the current contents of this text entity.
     * This property is updated by the update method.
     * @see update
     */
    double usedTextWidth;
    
    /**
     * Text height used by the current contents of this text entity.
     * This property is updated by the update method.
     * @see update
     */
    double usedTextHeight;
    
    /**
     * @return whether this container holds letter/glyph inserts
     */
    bool hasGlyphs();

    /**
     * Apply forced line breaks, perform auto word wrap, and apply list item formatting
     */
    RS_Vector layout(const RS_Vector &posnV, double leftMargin, double rightMargin);
    
   /** Load @param vecStarts with indexes where text switches from 'code sections'
     *  to 'display sections' and vice versa
     */
    void getSecnStarts(QList<unsigned int>& vecStarts);
    
    /** @return nsecn with the section that @param nchr falls within
      */
    unsigned short getSecn(unsigned int nchr, const QList<unsigned int>& vecStarts);    
    
	/** TODO - write this up
	  */
    bool stackFoundAt(unsigned short nsecn, const QList<unsigned int>& vecSecnStarts);

	/** Build tree segment for 'stacked' (fraction/superscript/subscript) text
	  * @return new assembly 
	  */
    RS_MText* buildStackAssy(const RS_MTextData& data, const LC_CodeTag& codeTag);
    
    /**
     * If calling entity is a glyph holder, find location within text where line break should occur, if 
     * any; if found, copy/split glyphs into two new mtext entities as appropriate, and replace the current 
     * glyph entities with the two new mtext entities. 
     * If calling entity is not a glyph holder, call wordwrap on its last child. On return, replace the 
     * last child with the two children the child now has.
     * @return true while further wordwrap is needed, then @return false.
     */
    bool wordwrap(double leftMarg, double rightMarg);
    
    /**
     * Used during wordwrap; avoid unwanted call to setText() during initial construction.
     */
    void resetFrom(
      const QList<RS_Entity *>::iterator& startEnts,
      const QList<RS_Entity *>::iterator& endEnts,
      const QString& _text
    );
      
    /**
     * @return whether line return is really needed.  FIXME: not certain about the logic used
     */
    bool wantsLineReturn(double leftMargin);
    
    /**
     * Add line entities to be drawn as underlines (only decoration currently implemented).
     */
    void setDecorations();
    
};

#endif
