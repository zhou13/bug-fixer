#include "model.h"
#include "basic.h"

pair<double, int> segments[MAX_ITEMS];
bool seg_le[MAX_ITEMS], seg_ri[MAX_ITEMS];
int segments_count;

TreeNode::TreeNode(const vector<Item*> pis, const Cuboid &space) {
    cuboid = pis[0]->GetCuboid();
    for (int i = 1; i < pis.size(); ++ i)
        cuboid |= pis[i]->GetCuboid();
    cuboid &= space;
    
    
    dm = -1;
    if (pis.size() <= LIMIT_NODE_ITEMS) {
        items = pis;
        child[0] = child[1] = NULL;
        return;
    }
   // printf("build pis.size()=%d   ", pis.size()); fflush(stdout);
    int bestfunc = (int)1E9;
    for (int kd = 0; kd < 3; ++ kd) {
        segments_count = 0;
        for (int i = 0; i < pis.size(); ++ i) {
            pair<double, double> cur = pis[i]->GetMinMaxDimension(kd);
            segments[segments_count ++] = make_pair(cur.first, +1);
            segments[segments_count ++] = make_pair(cur.second, -1);
        }
        sort(segments, segments+segments_count);
        int le = 0, ri = pis.size();
        for (int i = 0; i < segments_count; ++ i) {
            int curfunc = max(le, ri);
            if (i > 0 && curfunc < bestfunc) {
                bestfunc = curfunc;
                dm = kd;
                cut = (segments[i].first+segments[i-1].first) * 0.5;
            }
            if (segments[i].second > 0)
                ++ le;
            else
                -- ri;
        }
    }
    if (dm < 0)
        throw string("TreeNode::TreeNode(): bestdm < 0");
        
   // printf("bestfunc=%d", bestfunc); fflush(stdout);
    
    if ((double)bestfunc >= (double)pis.size() * 0.8) {
        dm = -1;
        items = pis;
        child[0] = child[1] = NULL;
        return;
    }
    vector<Item*> pl, pr, pe;
    for (int i = 0; i < pis.size(); ++ i) {
        pair<double, double> cur = pis[i]->GetMinMaxDimension(dm);
        bool bl = (cur.first + EPSI_BUILD_SP < cut);
        bool br = (cur.second > cut + EPSI_BUILD_SP);
        if (bl) pl.push_back(pis[i]);
        if (br) pr.push_back(pis[i]);
        if (!bl && !br) pe.push_back(pis[i]);
    }
    for (int i = 0; i < pe.size(); ++ i)
        if (pl.size() < pr.size())
            pl.push_back(pe[i]);
        else
            pr.push_back(pe[i]);
   // printf("   lsize=%d rsize=%d\n", pl.size(),pr.size()); fflush(stdout);
    /*
    pl.resize(cl), pr.resize(cr);
    cl = 0, cr = 0;
    for (int i = 0; i < pis.size(); ++ i) {
        if (seg_le[i]) pl[cl ++] = pis[i];
        if (seg_ri[i]) pr[cr ++] = pis[i];
    }*/
    Cuboid c1, c2;
    cuboid.CutInto(dm, cut, &c1, &c2);
    child[0] = new TreeNode(pl, c1);
    child[1] = new TreeNode(pr, c2);
}

bool TreeNode::CheckIntersect(const Point &viewpoint, const Point &direction, int current_moment) {
    if (!cuboid.CheckRay(viewpoint, direction))
        return false;
    if (dm < 0) {
        for (int i = 0; i < items.size(); ++ i)
            if (items[i]->last_visited != current_moment) {
                items[i]->last_visited = current_moment;
                if (items[i]->CheckIntersect(viewpoint, direction))
                    return true;
            }
        return false;
    }
    int cx = (viewpoint.GetDimension(dm) < cut ? 0 : 1);
    bool another_side = (direction.GetDimension(dm) * (cx == 0 ? +1.0 : -1.0)) > EPSI_DIV;
    if (child[cx]->CheckIntersect(viewpoint, direction, current_moment))
        return true;
    if (another_side)
        if (child[1-cx]->CheckIntersect(viewpoint, direction, current_moment))
            return true;
    return false;
}


void TreeNode::GetIntersection(const Point &viewpoint, const Point &direction, int current_moment, Intersection *isc) {
    if (!cuboid.CheckRay(viewpoint, direction))
        return;
    if (dm < 0) {
        Point point;
        double t;
        IntersectionDetails details;
        for (int i = 0; i < items.size(); ++ i)
            if (items[i]->last_visited != current_moment) {
                items[i]->last_visited = current_moment;
                if (items[i]->GetIntersection(viewpoint, direction, &point, &t, &details) && (isc->item == NULL || t < isc->t))
                    isc->item = items[i], isc->point = point, isc->t = t, isc->details = details;
            }
        return;      
    }
    int cx = (viewpoint.GetDimension(dm) < cut ? 0 : 1);
    bool another_side = (direction.GetDimension(dm) * (cx == 0 ? +1.0 : -1.0)) > EPSI_DIV;
    
    child[cx]->GetIntersection(viewpoint, direction, current_moment, isc);
    if (another_side)
        if (isc->item == NULL || (isc->item != NULL && (isc->point.GetDimension(dm) < cut ? 0 : 1) != cx))
            child[1-cx]->GetIntersection(viewpoint, direction, current_moment, isc);
}




