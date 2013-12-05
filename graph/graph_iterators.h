#ifndef __graph_iterators_h__
#define __graph_iterators_h__

namespace indigo 
{
   class Graph;
   class Vertex;
   struct Edge;

   class AutoIterator
   {
   public:
      AutoIterator( int idx );

      virtual int operator* () const;

      virtual bool operator!= ( const AutoIterator &other ) const;

      virtual AutoIterator & operator++ () = 0;

   protected:
      int _idx;
   };

   class VertexIter : public AutoIterator
   {
   public:
      VertexIter (Graph &owner, int idx);

      VertexIter & operator++ ();

   private:
      Graph &_owner;
   };

   class VerticesAuto
   {
   public:
      VerticesAuto (Graph &owner);

      VertexIter begin ();

      VertexIter end ();

   private:
      Graph &_owner;
   };

   class EdgeIter : public AutoIterator
   {
   public:
      EdgeIter (Graph &owner, int idx);

      EdgeIter & operator++ ();

   private:
      Graph &_owner;
   };

   class EdgesAuto
   {
   public:
      EdgesAuto (Graph &owner);

      EdgeIter begin ();

      EdgeIter end ();

   private:
      Graph &_owner;
   };

   class NeighborIter : public AutoIterator
   {
   public:
      NeighborIter(const Vertex &owner, int idx);

      NeighborIter & operator++ ();

   private:
      const Vertex &_owner;
   };

   class NeighborsAuto
   {
   public:
      NeighborsAuto ( const Vertex &owner);

      NeighborIter begin ();

      NeighborIter end ();

   private:
      const Vertex &_owner;
   };
};

#endif //__graph_iterators_h__