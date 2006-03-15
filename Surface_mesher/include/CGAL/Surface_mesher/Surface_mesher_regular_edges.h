// Copyright (c) 2003-2006  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Steve Oudot, David Rey, Mariette Yvinec, Laurent Rineau, Andreas Fabri

#ifndef CGAL_SURFACE_MESHER_REGULAR_EDGES_H
#define CGAL_SURFACE_MESHER_REGULAR_EDGES_H

#include <CGAL/Surface_mesher/Surface_mesher.h>
#include <CGAL/utility.h>
#include <CGAL/circulator.h>
#include <set>

namespace CGAL {

  namespace Surface_mesher {

  template <
    class C2T3,
    class Surface,
    class SurfaceMeshTraits,
    class Criteria,
    typename SMB = 
      Surface_mesher_base<C2T3, Surface, SurfaceMeshTraits, Criteria>
  >
  class Surface_mesher_regular_edges_base
    : public SMB 
  {
    public:
      typedef C2T3 C2t3;
      typedef typename C2T3::Triangulation Tr;
      typedef typename Tr::Geom_traits GT;
      typedef typename GT::FT FT;
      typedef typename Tr::Point Point;
      typedef typename Tr::Facet Facet;
      typedef typename Tr::Edge Edge;
      typedef typename Tr::Cell_handle Cell_handle;
      typedef typename Tr::Vertex_handle Vertex_handle;
      typedef std::pair<Vertex_handle, Vertex_handle> EdgeVV;
      typedef typename Triangulation_mesher_level_traits_3<Tr>::Zone Zone;
      typedef std::list<Edge> Edges;
      typedef std::list<Facet> Facets;
      typedef Const_circulator_from_container<Facets> Facet_circulator;
      typedef typename Tr::Finite_edges_iterator Finite_edges_iterator;

    protected:
      std::set <std::pair <Vertex_handle, Vertex_handle> > bad_edges;

    protected:
      bool is_in_complex(const C2t3& c, const Facet& f) const {
	return (c.face_type(f) != C2t3::NOT_IN_COMPLEX);
      }

      // computes and return an ordered pair of Vertex
      EdgeVV make_edgevv(const Vertex_handle vh1,
			 const Vertex_handle vh2) const {
	if (vh1 < vh2) {
	  return std::make_pair(vh1, vh2);
	}
	else {
	  return std::make_pair(vh2, vh1);
	}
      }

      // computes and returns the Edge type object from the EdgeVV object
      // use tr.is_edge(...)
      Edge edgevv_to_edge(const EdgeVV& arete) const {
    	Vertex_handle sommet1 = arete.first;
    	Vertex_handle sommet2 = arete.second;
    	Cell_handle c;
	int index1, index2;

	SMB::tr.is_edge(sommet1, sommet2, c, index1, index2);

    	return make_triple( c, index1, index2 );
      }

      // computes and returns the EdgeVV type object from the Edge object
      EdgeVV edge_to_edgevv(const Edge& arete) const {
	return make_edgevv(arete.first->vertex(arete.second),
			   arete.first->vertex(arete.third) );
      }



      // computes and returns the list of Edges in a given Facet side
      Edges edges_in_facet(const Facet& cote) const {

	Edges loe;
	Cell_handle c = cote.first;
	int i = cote.second;

	for (int j = 0; j < 3; j++) {
	  for (int k = j + 1; k < 4; k++) {
	    if ( (i != j) && (i != k) ){
	      loe.push_back(make_triple(c, j, k));
	    }
	  }
	}
	return loe;
      }
    
    FT compute_distance_to_facet_center(const Facet& f,
					const Vertex_handle v) const {
      const Point& fcenter = f.first->get_facet_surface_center(f.second);
      const Point& vpoint = v->point();

      return SMB::tr.geom_traits().compute_squared_distance_3_object()(fcenter,
								  vpoint );
    }


      Facet biggest_incident_facet_in_complex(const Edge& arete) const {
	// Find the first facet in the incident facets
        // of the edge which is in the Complex
	// use the list of incident facets in the complex
	Facet_circulator fcirc = SMB::c2t3.incident_facets(arete);
	Facet first_facet = *fcirc;
	Facet biggest_facet = first_facet;

	for (++fcirc;
	     (*fcirc != first_facet) &&
	       (*fcirc != SMB::mirror_facet(first_facet));
	     ++fcirc) {
	  Vertex_handle fev = edge_to_edgevv(arete).first;
	  // is the current facet bigger than the current biggest one
	  if ( compute_distance_to_facet_center(*fcirc, fev) >
	       compute_distance_to_facet_center(biggest_facet,
                                                fev) ) {
	    biggest_facet = *fcirc;
	  }
	  else { // @TODO: il ne faut pas aller voir des deux cotes: c'est
		 // le meme centre de facet!!!
	    Facet autre_cote = SMB::mirror_facet(*fcirc);
	    // is the current facet bigger than the current biggest one
	    if ( compute_distance_to_facet_center(autre_cote, fev) >
		 compute_distance_to_facet_center(biggest_facet, fev) ) {
	      biggest_facet = autre_cote;
	    }
	  }
	}
	return biggest_facet;
      }

      ///////////////////////
      // For before_insertion

      // Actions to perform on a facet inside the conflict zone
      void handle_facet_inside_conflict_zone (const Facet& f) {
	Facet cote = f;
	Facet autre_cote = SMB::mirror_facet(cote);

	if ( SMB::c2t3.face_status(cote) !=
	     C2t3::NOT_IN_COMPLEX ) {

	  Edges loe = edges_in_facet(cote);
	  // foreach edge of cote
	  for ( typename Edges::iterator eit = loe.begin();
		eit != loe.end();
		++eit) {
	    bad_edges.erase( edge_to_edgevv(*eit) );
	  }
	}
      }

      // Action to perform on a facet on the boundary of the conflict zone
      void handle_facet_on_boundary_of_conflict_zone (const Facet& f) {
	// perform the same operations as for an internal facet
	handle_facet_inside_conflict_zone (f);
      }

    public:
      Surface_mesher_regular_edges_base(C2T3& c2t3,
                                        Surface& surface,
                                        SurfaceMeshTraits mesh_traits,
                                        Criteria& criteria)
        : SMB(c2t3, surface, mesh_traits, criteria)
    {
#ifdef CGAL_SURFACE_MESHER_DEBUG_CONSTRUCTORS
      std::cerr << "CONS: Surface_mesher_regular_edges_base\n";
#endif
    }

    // Initialization function
    void scan_triangulation_impl() {
      scan_triangulation_impl(true);
    }

    void scan_triangulation_impl(const bool withBoundary) {
      SMB::scan_triangulation_impl();
      std::cout << "scanning edges..." << std::endl;
      for (Finite_edges_iterator eit = SMB::tr.finite_edges_begin(); eit !=
	     SMB::tr.finite_edges_end(); ++eit) {
	if ( (SMB::c2t3.face_status(*eit)
	      == C2t3::SINGULAR) ||
	     ( (!withBoundary) &&
	       (SMB::c2t3.face_status(*eit)
		== C2t3::BOUNDARY) ) ) {
	  bad_edges.insert( edge_to_edgevv(*eit) );
	}
      }
    }

    // Tells whether there remain elements to refine
    bool no_longer_element_to_refine_impl() const {
      return (SMB::facets_to_refine.empty() && bad_edges.empty());
    }

    // Returns the next element to refine
    Facet get_next_element_impl() {

      if (!SMB::facets_to_refine.empty()) {
	return SMB::facets_to_refine.front()->second;
      }
      else {
	Edge first_bad_edge = edgevv_to_edge(*(bad_edges.begin()));
	return biggest_incident_facet_in_complex(first_bad_edge);
      }
    }

    void before_insertion_impl(const Facet&, const Point& s,
			       Zone& zone) {
      for (typename Zone::Facets_iterator fit =
	     zone.internal_facets.begin();
	   fit != zone.internal_facets.end();
	   ++fit)
	handle_facet_inside_conflict_zone (*fit);

      for (typename Zone::Facets_iterator fit =
	     zone.boundary_facets.begin(); fit !=
	     zone.boundary_facets.end(); ++fit)
	handle_facet_on_boundary_of_conflict_zone (*fit);

      SMB::before_insertion_impl(Facet(), s, zone);
    }

    void after_insertion_impl(const Vertex_handle v,
			      const bool withBoundary) {
      SMB::after_insertion_impl(v);

      // search for incident facets around v
      Facets facets;
      SMB::tr.incident_facets (v, std::back_inserter(facets));

      // foreach f in star(v)
      for (typename Facets::iterator fit = facets.begin();
	   fit != facets.end();
	   ++fit) {
	Facet cote = *fit;
	Edges loe = edges_in_facet(cote);

	// foreach edge of cote
	for ( typename Edges::iterator eit = loe.begin();
	      eit != loe.end();
	      ++eit ) {
	  // test if edge is in Complex
	  if ( SMB::c2t3.face_status(*eit)
	       != C2t3::NOT_IN_COMPLEX ) {
	    // test if edge is not regular to store it as a "bad_edge"
	    // e.g. more than or equal to 3 incident facets (SINGULAR)
	    // or less than or equal to 1
	    // (BOUNDARY only, because ISOLATED is NA)
	    // This test is not efficient because
	    // edges are tried to be inserted several times
	    // TODO one day: test if the edge is still singular
	    if ( (SMB::c2t3.face_status(*eit)
		  == C2t3::SINGULAR) ||
		 ( (!withBoundary) &&
		   (SMB::c2t3.face_status(*eit)
		    == C2t3::BOUNDARY) ) ) {
	      bad_edges.insert( edge_to_edgevv(*eit) );
	    }
	    else {
	      bad_edges.erase( edge_to_edgevv(*eit) ); // @TODO: pourquoi?!
	    }
	  }
	}
      }
    }

    void after_insertion_impl(const Vertex_handle v) {
      after_insertion_impl(v, true);
    }

    std::string debug_info() const
    {
      std::stringstream s;
      s << SMB::debug_info() << "," << bad_edges.size();
      return s.str();
    }
  };  // end Surface_mesher_regular_edges_base

  template <
    typename C2T3,
    typename Surface,
    typename SurfaceMeshTraits,
    typename Criteria
    >
  class Surface_mesher_regular_edges 
    : public Surface_mesher<C2T3, Surface, SurfaceMeshTraits, Criteria,
                            Surface_mesher_regular_edges_base<C2T3,
                                                              Surface,
                                                              SurfaceMeshTraits,
                                                              Criteria> >
  {
    typedef Surface_mesher<C2T3, Surface, SurfaceMeshTraits, Criteria,
                           Surface_mesher_regular_edges_base<C2T3,
                                                             Surface,
                                                             SurfaceMeshTraits,
                                                             Criteria> > SM;
  public:
    Surface_mesher_regular_edges(C2T3& c2t3,
                                 Surface& surface,
                                 SurfaceMeshTraits mesh_traits,
                                 Criteria& criteria)
      : SM(c2t3, surface, mesh_traits, criteria)
    {
#ifdef CGAL_SURFACE_MESHER_DEBUG_CONSTRUCTORS
      std::cerr << "CONS: Surface_mesher_regular_edges\n";
#endif
    }
  };  // end Surface_mesher_regular_edges

  }  // end namespace Surface_mesher

}  // end namespace CGAL


#endif // CGAL_SURFACE_MESHER_REGULAR_EDGES_H

