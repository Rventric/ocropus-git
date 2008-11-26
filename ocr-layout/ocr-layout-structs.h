// -*- C++ -*-

// Copyright 2006-2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz 
// or its licensors, as applicable.
// 
// You may not use this file except under the terms of the accompanying license.
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
// Project: OCRopus
// File: ocr-layout-rast.h
// Purpose: Extract textlines from a document image using RAST
//          For more information, please refer to the paper:
//          T. M. Breuel. "High Performance Document Layout Analysis",
//          Symposium on Document Image Understanding Technology, Maryland.
//          http://pubs.iupr.org/DATA/2003-breuel-sdiut.pdf
// Responsible: Faisal Shafait (faisal.shafait@dfki.de)
// Reviewer: 
// Primary Repository: 
// Web Sites: www.iupr.org, www.dfki.de

#ifndef h_ocrlayoutstructs__
#define h_ocrlayoutstructs__

#include "ocropus.h"

namespace ocropus {

    /////////////////////////////////////////////////////////////////////
    ///
    /// \struct TextLineParam
    /// Purpose: Textline parameters
    ///
    //////////////////////////////////////////////////////////////////////
    
    struct TextLineParam {
        float c,m,d; // c is y-intercept, m is slope, d is the line of descenders
        void print(FILE *stream=stdout){
            fprintf(stream,"%.3f %f %.2f\n",c,m,d);
        }
    };

    /////////////////////////////////////////////////////////////////////
    ///
    /// \struct TextLine
    /// Purpose: Textline bounding box and it attributes
    ///
    //////////////////////////////////////////////////////////////////////
    
    struct TextLine : TextLineParam{
        TextLine(){
        }
        TextLine(TextLineParam &tl){
            c = tl.c;
            m = tl.m;
            d = tl.d;
        }
        int   xheight;
        colib::rectangle  bbox;
        void print(FILE *stream=stdout){
            fprintf(stream,"%d %d %d %d ",bbox.x0,bbox.y0,bbox.x1,bbox.y1);
            fprintf(stream,"%.3f %f %.2f %d\n",c,m,d,xheight);
        }
    };

    /////////////////////////////////////////////////////////////////////
    ///
    /// \struct Line
    /// Purpose: Textlines extended to the nearest column separators
    ///
    //////////////////////////////////////////////////////////////////////
    
    struct line{
        float c,m,d; // c is y-intercept, m is slope, d is the line of descenders
        float start,end,top,bottom; // start and end of line segment
        float istart,iend; //actual start and end of line segment in the image
        float xheight;

        line() {}
        line(TextLine &tl);
        TextLine getTextLine();
    };

}

#endif
