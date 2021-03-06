//Performs the D* Lite algorithm on a given mesh
//Daanyaal du Toit
//Created: 7 July 2013
//Last Modified: 23 September 2013

#ifndef PATHFINDER_H_INCLUDED
#define PATHFINDER_H_INCLUDED

#include "boost/heap/d_ary_heap.hpp"
#include "NodeManager.h"
#include <iostream>
#include <iomanip>

namespace ADStar{

template<class N, class E>
class Pathfinder{

    public:
    //typedefs

        typedef typename boost::graph_traits<WeightedGraph>::out_edge_iterator oe_iterator;
        typedef typename boost::graph_traits<WeightedGraph>::in_edge_iterator ie_iterator;

        //Constructor
        Pathfinder(E & g): manager(g){}

    private:
        //typedefs
        typedef typename boost::heap::d_ary_heap<N, boost::heap::mutable_<true>
                                , boost::heap::arity<4>
                                , boost::heap::compare<std::greater<N> >
                                > Dary_queue;
        typedef typename Dary_queue::handle_type q_handle_type;
        typedef typename NodeManager<N, E>::node_handle node_handle;

        //Pathfinder members
        NodeManager<N, E> manager;
        std::vector<CGAL::Point_3<K> > path;
        //E & graph;
        node_handle last, start, end;
        float km, cost;
        std::map<node_handle, q_handle_type> on_queue;          //maps from node indices to nodes on the heap
        Dary_queue p_queue;

    public:

    float getCost(){return cost;}
    int getFuncs(){return manager.numFuncs;}

//---------------------------------------------------------------------------------
//Main pathfinding method
//---------------------------------------------------------------------------------

    void findPath(node_handle s, node_handle e){

        cost = 0;

        //initialise pathfinder
        start = s;
        end = e;

        last = start;
        path.clear();
        on_queue.clear();
        p_queue.clear();
        manager.setRHS(end, 0);
        manager.setG(end, manager.heuristic(start, end));
        //std::cout << "End G:" << manager.getG(end) << std::endl;

        on_queue[end] = p_queue.push(N(end, manager.heuristic(start, end), 0));

        //perform initial path computation
        computePath();

    }

//---------------------------------------------------------------------------------
//Move along the least expensive path
//---------------------------------------------------------------------------------

    void move(){

        if(start != end){

        //exit if no path
        if(manager.getRHS(start) == INFINITY){

            std::cout << "No path found." << std::endl;
            return;

        }

        /*if(start == 34003)
            std::cout << "Key of 34002 : " << graph[34002].key << std::endl;*/

        //calc min traversal to one of cur's successors and assign to start
        node_handle min_succ = manager.getMinSucc(start);

        /*std::cout << "Min Succ = " << min_succ << " g = " << manager.getG(min_succ);
        char a;
        std::cin >> a;*/

        //move to succesor
        path.push_back(start);
        cost += manager.getCost(start, min_succ);
        //std::cout << "Cost from " << start << " to " << min_succ << " = " << min_cost - graph[min_succ].key.first << std::endl;
        //std::cout << "Accum Cost = " << cost << std::endl;
        start = min_succ;
        //std::cout << "Moving to " << start << std::endl;

        if(start == end)
            path.push_back(start);

        }

        //std::cout << "Move complete" << std::endl;

    }

//---------------------------------------------------------------------------------
//Handle changes polled by the NodeUpdator
//---------------------------------------------------------------------------------

    void handleChanges(const std::deque<node_handle> & c){

        char a;

        if(!c.empty()){

            //std::cout << "Changing keys" << std::endl << "==================================" << std::endl;

            //for all new facets
            for(typename std::deque<node_handle>::const_iterator it = c.begin(); it != c.end(); ++it){

                std::vector<node_handle> preds = manager.getPreds(*it, true);

                for(typename std::vector<node_handle>::iterator pit = preds.begin(); pit != preds.end(); ++pit){

                //std::cout << "Trying to update " << (*pit)->point() << std::endl;
                //std::cout << "Has old key = " << (*pit)->key << std::endl;
                //std::cin >> a;

                if(*pit != end){

                    float newRHS = INFINITY;
                    //manager.setRHS(*pit, manager.getCost(*pit, (*pit)->vertex_begin()->facet()));
                    PathPoly_3::Halfedge_around_vertex_circulator fit = (*pit)->vertex_begin();
                    do{

                        if(fit->facet() != PathPoly_3::Facet_handle()){

                            //std::cout << "Adjacent to triangle" << std::endl;
                            costPair cp = manager.getCostPair(*pit, fit->facet());
                            newRHS = std::min(newRHS, cp.first);
                            if(newRHS < fit->facet()->pathCost){
                            fit->facet()->pathCost = newRHS;
                            fit->facet()->lowPoint = cp.second.first.point();
                            }

                            }
                        ++fit;

                    }
                    while(fit != (*pit)->vertex_begin());

                    manager.setRHS(*pit, newRHS);
                    //manager.setG(*pit, newRHS);

                    //std::cout << "RHS of " << (*pit)->point() << ": " << manager.getRHS(*pit) << std::endl;

                }

                //std::cout << "Key is now " << (*pit)->key << std::endl;
                //std::cin >> a;
                updateNode(*pit);
                //std::cout << "Key is now " << (*pit)->key << std::endl;
                //std::cin >> a;


                }

                //std::cout << "Heuristic for " << (*it)->point() << ": " << manager.heuristic(start, (*it)) << std::endl;

            }

            //std::cout << "Computing path" << std::endl;
            start->key.first = INFINITY;
            computePath();

        }

    }

//---------------------------------------------------------------------------------
//Get path and cost of path
//---------------------------------------------------------------------------------

    std::vector<CGAL::Point_3<K> > & getPathPoints(){return path;}

    std::vector<UpdateBundle> getPath(bool adjust = true){

        std::vector<UpdateBundle> facets;
        //std::cout << "Starting Path:" << std::endl << "================================================" << std::endl;
        //std::cout << "Nodes in mesh: " << manager.getEnvir().size_of_vertices() << std::endl;

        node_handle cur = start;
        node_handle tempCur;
        cost = start->key.second;
        path.clear();
        path.push_back(cur->point());
        bool interpol = false;
        PathPoly_3::Halfedge_handle intEdge;
        char a;

        while(cur != end){

            /*if(interpol){

                std::cout << "Processing interpolated point " << tempCur->point() << std::endl;
                PathPoly_3::Halfedge_around_vertex_circulator it = tempCur->vertex_begin();
                //std::cin >> a;
                costPair lowCost(INFINITY, intPair());
                PathPoly_3::Facet_handle lowTri;

                do{

                    if(it->facet() != PathPoly_3::Facet_handle()){
                        std::cout << "Processing triangle with weight " << it->facet()->weight << std::endl;
                        //std::cin >> a;
                        costPair newCost = manager.getCostPair(tempCur, it->facet(), true);
                        if(newCost.first < lowCost.first){

                        lowCost = newCost;
                        lowTri = it->facet();

                        }

                        }
                    ++it;

                }
                while(it != tempCur->vertex_begin());

                PathPoly_3::Halfedge_handle he1 = intEdge->next();
                PathPoly_3::Halfedge_handle he2 = intEdge->opposite()->next();

                PathPoly_3::Halfedge_around_facet_circulator lowTriFit = lowTri->facet_begin();

                do{

                    if(isEqual(*(lowTriFit->vertex()), *(he1->vertex()))){

                        UpdateBundle b;
                        b.handle = he1;
                        //cfacets.push_back(b);

                    }
                    else{

                        UpdateBundle b;
                        b.handle = he2;
                        //facets.push_back(b);

                    }

                }
                while(lowTriFit != lowTri->facet_begin());

                std::cout << std::setprecision(6) << "Next point: " << lowCost.second.first.point() << std::endl;
                //std::cin >> a;

                nextPair next = manager.getNextVert(lowTri, lowCost);
                interpol = next.first;
                path.push_back(lowCost.second.first.point());
                if(!interpol){

                    //Find point on original mesh
                    PathPoly_3::Facet_handle f = intEdge->facet();
                    PathPoly_3::Halfedge_around_facet_circulator fit1 = f->facet_begin();

                    do{

                        if(isEqual(*(fit1->vertex()), *(next.second.first))){

                            cur = fit1->vertex();
                            break;

                        }
                        ++fit1;

                    }
                    while(fit1 != f->facet_begin());

                    if(intEdge->opposite()->facet() != PathPoly_3::Facet_handle()){

                    f = intEdge->opposite()->facet();
                    PathPoly_3::Halfedge_around_facet_circulator fit2 = f->facet_begin();

                    do{

                        if(isEqual(*(fit2->vertex()), *(next.second.first))){

                            cur = fit2->vertex();
                            break;

                        }
                        ++fit2;

                    }
                    while(fit2 != f->facet_begin());

                }

                }
                else{
                    tempCur = next.second.first;
                    intEdge = next.second.second;
                    }

            }
            else*/{

                //std::cout << "Processing mesh point " << cur->point() << std::endl;
                //std::cin >> a;

                PathPoly_3::Halfedge_around_vertex_circulator it = cur->vertex_begin();
                costPair lowCost(INFINITY, intPair());
                PathPoly_3::Facet_handle lowTri;

                //get triangle with lowest cost func
                do{

                    //std::cout << "Processing triangle with weight " << it->facet()->weight << std::endl;
                    //std::cout << "Cost is " << lowCost.first << std::endl;

                    if(it->facet() != PathPoly_3::Facet_handle()){
                        //std::cin >> a;
                        costPair nextCost = manager.getCostPair(cur, it->facet(), adjust);
                        if(nextCost.first < lowCost.first){

                        lowCost = nextCost;
                        lowTri = it->facet();
                        it->opposite()->vertex()->newPoint = false;

                        }

                        //std::cout << "lowPoint of tri: " << lowCost.second.first.point() << std::endl;
                        }
                    ++it;

                }
                while(it != cur->vertex_begin());

                /*std::cout << std::setprecision(6) << "Next point: " << lowCost.second.first.point()
                            << " with cost: " << lowCost.first << std::endl;*/
                //std::cin >> a;

                nextPair next = manager.getNextVert(lowTri, lowCost);
                interpol = next.first;
                path.push_back(lowCost.second.first.point());

                //determine need for temp cells if point interpolated
                if(!interpol){

                    cur = next.second.first;
                    }
                else{

                    //add point to be adapted
                    UpdateBundle b;
                    b.handle = lowCost.second.second;
                    b.point = lowCost.second.first.point();

                    /*std::cout << "===============\nNext from handle: " << b.handle->opposite()->next()->vertex()->point() << "\n====================="
                            << std::endl;*/

                    //std::cin >> a;

                    cur = b.handle->opposite()->next()->vertex();

                    interpol = false;

                    float edge_length = sqrt(CGAL::Vector_3<K>(lowCost.second.second->vertex()->point()
                                                      , lowCost.second.second->opposite()->vertex()->point()).squared_length());

                    float range = lowCost.second.second->vertex()->key.first - lowCost.second.second->opposite()->vertex()->key.first;

                    if(lowCost.second.second->vertex()->newPoint)
                        range -= lowCost.second.second->vertex()->key.first/(2*lowCost.second.second->vertex()->key.first);

                    /*std::cout << "Interpolate between " << lowCost.second.second->vertex()->point() << "with key " << lowCost.second.second->vertex()->key.first
                            << " and " << lowCost.second.second->opposite()->vertex()->point() << "with key " << lowCost.second.second->opposite()->vertex()->key.first << std::endl;
                    std::cout << "Range: " << range << std::endl;
                    std::cout << "Length: " << edge_length << std::endl;
                    std::cout << "Ratio: " << sqrt(CGAL::Vector_3<K>(b.point, lowCost.second.second->vertex()->point()).squared_length()) / edge_length << std::endl;
                    */
                    b.cost = lowCost.second.second->opposite()->vertex()->key.first + range * sqrt(CGAL::Vector_3<K>(b.point, lowCost.second.second->vertex()->point()).squared_length()) / edge_length;

                    /*b.cost = lowCost.second.second->vertex()->key.first
                                - sqrt(CGAL::Vector_3<K>(lowCost.second.second->vertex()->point(), next.second.first->point()).squared_length() / edge_length)
                                * range;*/

                    //b.cost = lowCost.first;

                    facets.push_back(b);
                    path.push_back(b.handle->opposite()->next()->vertex()->point());

                    //std::cout << "Adding bundle: " << b.point << ", " << b.cost << std::endl;
                    //std::cin >> a;

                    tempCur = next.second.first;
                    intEdge = next.second.second;
                    }

            }

        }
        //path.push_back(cur->point());
        std::cout << "Shortest Path:" << std::endl;
        for(typename std::vector<CGAL::Point_3<K> >::iterator it = path.begin(); it != path.end(); ++it)
            std::cout << "(" << *it << ") ";
        std::cout << std::endl;

        std::cout << "Path cost: " << cost << std::endl;

        return facets;

    }

    inline bool atEnd(){return start == end;}

    private:

//---------------------------------------------------------------------------------
//Change the key of graph node
//---------------------------------------------------------------------------------

    void updateNode(node_handle index){

        if(manager.getG(index) != manager.getRHS(index)){  //if key is inconsistent
            if(on_queue.find(index) != on_queue.end()){         //if already on queue
            //update node priority

            node_key new_key = manager.computeKey(start, index);
            p_queue.update(on_queue[index], N(index, new_key));
            index->key = new_key;
            //std::cout << "Updating key for node " << index << " to " << graph[index].key << std::endl;

            }
            else{
            //insert node onto queue
            node_key new_key = manager.computeKey(start, index);
            on_queue[index] = p_queue.push(N(index, new_key));
            index->key = new_key;
            //std::cout << "Inserting node " << index->point() << " with key " << new_key << std::endl;
            }

        }
        else if(on_queue.find(index) != on_queue.end()){        //if key consistent and on queue
            //remove node from queue
            p_queue.erase(on_queue[index]);
            on_queue.erase(index);
            //std::cout << "Removing node " << index << std::endl;

        }

    }

//---------------------------------------------------------------------------------
//Finds a path from the goal to the start
//---------------------------------------------------------------------------------

    void computePath(){

    //std::cout << "Computing path:" << std::endl << "========================================" << std::endl;
    //for(typename std::map<node_handle, q_handle_type>::iterator it = on_queue.begin(); it != on_queue.end(); ++it)
    //    std::cout << "(" << it->first->point() << ") = " << manager.computeKey(start, it->first) << std::endl;

    while(!p_queue.empty() && (manager.computeKey(start,p_queue.top().index) < manager.computeKey(start, start) || std::fabs(manager.getRHS(start) - manager.getG(start)) > 1e-5)){

        //std::cout << "Processing Node " << p_queue.top().index->point() << std::endl;
        //char a;
        //std::cin >> a;
        //compare old and new keys for changes
        node_handle top_index = p_queue.top().index;
        node_key key_old(p_queue.top().getG(), p_queue.top().getRHS());

        p_queue.pop();
        on_queue.erase(top_index);

        //std::cout << "Key of " << top_index->point() << " : " << key_old << std::endl;

        //char a;
        //std::cin >> a;

        //if key has changed
        /*if(key_old < key_new){

            //update the current node on the queue
            //std::cout << "Updating node " << top_index->point() << " with key " << key_new << std::endl;
            top_index->key = key_new;
            p_queue.update(on_queue[top_index], N(top_index, key_new));

            }
        //if the graph's key has different values
        else */if(manager.getG(top_index) > manager.getRHS(top_index)){

            //std::cout << "Adjusting inconsistent key " << key_old << " of " << top_index->point() << std::endl;
            //update node and remove from queue
            manager.setG(top_index, manager.getRHS(top_index));

            //std::cin >> a;

            std::vector<node_handle> toProcess = manager.getPreds(top_index, false);
            //std::cout << toProcess.size() << std::endl;
            //std::cin >> a;
            for(typename std::vector<node_handle>::iterator it = toProcess.begin(); it != toProcess.end(); ++it){

                //std::cout << top_index->point() << " != " << (*it)->point() << " : ";
                //std::cout << (*it != end) << std::endl;

                /*std::cout << "Setting RHS of " << (*it)->point();
                std::cout << " to min(" << manager.getRHS(*it) << ", ";
                std::cout << manager.getG(top_index) << " + " << manager.getCost(*it, top_index) << ")" << std::endl;*/

                if(*it != end){

                    PathPoly_3::Halfedge_around_vertex_circulator vit = (*it)->vertex_begin();
                    float newRHS = INFINITY;
                    do{

                        if(vit->facet() != PathPoly_3::Facet_handle()){

                            //std::cout << "Processing " << (*it)->point() << " on triangle with weight " << vit->facet()->weight << std::endl;
                            costPair cp = manager.getCostPair(*it, vit->facet());
                            newRHS = std::min(newRHS, cp.first);
                            if(newRHS < vit->facet()->pathCost){
                            vit->facet()->pathCost = newRHS;
                            vit->facet()->lowPoint = cp.second.first.point();

                            //std::cout << "Gave facet cost " << newRHS << " and point " << cp.second.first.point() << std::endl;

                            }

                            }

                        ++vit;

                    }
                    while(vit != (*it)->vertex_begin());

                    manager.setRHS(*it, std::min(manager.getRHS(*it), newRHS));

                    //std::cout << "RHS of " << (*it)->point() << " set to " << manager.getRHS(*it) << std::endl;

                }


                //std::cin >> a;

                updateNode(*it);

                }

        }
        else{

        //float old_g = manager.getG(top_index);
        manager.setG(top_index, INFINITY);

        //std::cout << "Key of " << top.index << " : " << graph[top.index].key << std::endl;

        std::vector<node_handle> toProcess = manager.getPreds(top_index, true);
        for(typename std::vector<node_handle>::iterator it = toProcess.begin(); it != toProcess.end(); ++it){

            if(*it != end){

            PathPoly_3::Halfedge_around_vertex_circulator vit = (*it)->vertex_begin();
            float newRHS = INFINITY;

            do{

                if(vit->facet() != PathPoly_3::Facet_handle()){

                    costPair cp = manager.getCostPair(*it, vit->facet());
                    //std::cout << "Processing " << (*it)->point() << " on triangle with weight " << vit->facet()->weight << std::endl;
                    newRHS = std::min(newRHS, cp.first);
                    if(newRHS < vit->facet()->pathCost){
                    vit->facet()->pathCost = newRHS;
                    vit->facet()->lowPoint = cp.second.first.point();

                    }

                }

            }
            while(vit != (*it)->vertex_begin());

            manager.setRHS(*it, std::min(manager.getRHS(*it), newRHS));

            }

            updateNode(*it);

        }

        }

        //std::cout << "Key of " << top_index->point() << " = " << top_index->key << std::endl;

    }

    //std::cout << "StartG = " << manager.getG(start);
    //std::cout << std::endl;

    }

};

}

#endif // PATHFINDER_H_INCLUDED
