/****************************************************************************
 * MeshLab                                                           o o     *
 * A versatile mesh processing toolbox                             o     o   *
 *                                                                _   O  _   *
 * Copyright(C) 2005                                                \/)\/    *
 * Visual Computing Lab                                            /\/|      *
 * ISTI - Italian National Research Council                           |      *
 *                                                                    \      *
 * All rights reserved.																											 *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
 * for more details.                                                         *
 *                                                                           *
 ****************************************************************************/
/****************************************************************************
  History
$Log$
Revision 1.8  2007/03/27 12:20:18  cignoni
Revamped logging iterface, changed function names in automatic parameters, better selection handling

Revision 1.7  2007/03/20 16:23:10  cignoni
Big small change in accessing mesh interface. First step toward layers

Revision 1.6  2007/03/03 02:03:51  cignoni
Removed bug on simplification of selected faces

Revision 1.5  2007/02/25 21:31:49  cignoni
new parameters for quadric simplification

Revision 1.4  2007/01/19 09:12:39  cignoni
Added parameters for quality,selection and boundary preservation

Revision 1.3  2006/10/19 07:34:24  cignoni
added callback

Revision 1.2  2006/10/15 17:08:52  cignoni
typenames and qualifiers for gcc compliance

Revision 1.1  2006/10/10 21:13:08  cignoni
Added remove non manifold and quadric simplification filter.

****************************************************************************/
#include <QtGui>

#include <math.h>
#include <stdlib.h>

#include <limits>

#include "meshfilter.h"
#include <vcg/complex/trimesh/update/position.h>
#include <vcg/complex/trimesh/update/bounding.h>
#include <vcg/complex/trimesh/update/selection.h>
#include <vcg/complex/local_optimization.h>
#include <vcg/complex/local_optimization/tri_edge_collapse_quadric.h>
#include <vcg/container/simple_temporary_data.h>
#include "quadric_simp.h"

using namespace vcg;
using namespace std;

void QuadricSimplification(CMeshO &m,int  TargetFaceNum, bool Selected, CallBackPos *cb)
{
  math::Quadric<double> QZero;
  QZero.SetZero();
  tri::QuadricTemp TD(m.vert,QZero);
  tri::QHelper::TDp()=&TD;

	// we assume that the caller has already set up the tri::MyTriEdgeCollapse::Params() class
	tri::TriEdgeCollapseQuadricParameter & pp = tri::MyTriEdgeCollapse::Params();
  
  if(Selected) // simplify only inside selected faces
  {
    // select only the vertices having ALL incident faces selected
    tri::UpdateSelection<CMeshO>::VertexFromFaceStrict(m);

    // Mark not writable un-selected vertices
    CMeshO::VertexIterator  vi;
    for(vi=m.vert.begin();vi!=m.vert.end();++vi) if(!(*vi).IsD())
          if(!(*vi).IsS()) (*vi).ClearW();
                      else (*vi).SetW();
  }

  if(pp.PreserveBoundary && !Selected) 
	{
    pp.FastPreserveBoundary=true;
		pp.PreserveBoundary = false;
	}
		
  if(pp.NormalCheck) pp.NormalThrRad = M_PI/4.0;
	
	
  vcg::LocalOptimization<CMeshO> DeciSession(m);
	cb(1,"Initializing simplification");
	DeciSession.Init<tri::MyTriEdgeCollapse >();

	if(Selected)
		TargetFaceNum= m.fn - (m.sfn-TargetFaceNum);
	DeciSession.SetTargetSimplices(TargetFaceNum);
	DeciSession.SetTimeBudget(0.1f); // this allow to update the progress bar 10 time for sec...
//  if(TargetError< numeric_limits<double>::max() ) DeciSession.SetTargetMetric(TargetError);
  int startFn=m.fn;
  int faceToDel=m.fn-TargetFaceNum;
 while( DeciSession.DoOptimization() && m.fn>TargetFaceNum )
 {
   cb(100-100*(m.fn-TargetFaceNum)/(faceToDel), "Simplifying...");
 };

	DeciSession.Finalize<tri::MyTriEdgeCollapse >();
  
  if(Selected) // Clear Writable flags 
  {
    CMeshO::VertexIterator  vi;
    for(vi=m.vert.begin();vi!=m.vert.end();++vi) 
      if(!(*vi).IsD()) (*vi).SetW();
  }
}