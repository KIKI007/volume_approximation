// VolEsti (volume computation and sampling library)

// Copyright (c) 2018 Vissarion Fisikopoulos, Apostolos Chalkis

//Contributed and/or modified by Apostolos Chalkis, as part of Google Summer of Code 2018 program.

// VolEsti is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// VolEsti is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// See the file COPYING.LESSER for the text of the GNU Lesser General
// Public License.  If you did not receive this file along with HeaDDaCHe,
// see <http://www.gnu.org/licenses/>.


#ifndef GAUSSIAN_SAMPLERS_H
#define GAUSSIAN_SAMPLERS_H

//#include <stdio.h>
//#include <math.h>

NT eval_exp(Point p, NT a){
    return std::exp(-a*p.squared_length());
}


NT get_max(Point l, Point u, NT a_i){
    NT res;
    Point a = -1.0*l;
    Point bef = u-l;
    Point b = (1.0/std::sqrt((bef).squared_length()))*bef;
    Point z = (a.dot(b)*b)+l;
    NT low_bd = (l[0]-z[0])/b[0];
    NT up_bd = (u[0]-z[0])/b[0];
    if(low_bd*up_bd>0){
    //if(std::signbit(low_bd)==std::signbit(up_bd)){
        res = std::max(eval_exp(u,a_i),eval_exp(l,a_i));
    }else{
        res = eval_exp(z,a_i);
    }

    return res;
}


NT get_max_coord(NT l, NT u, NT a_i){
    if(l<0.0 && u>0.0){
        return 1.0;
    }
    return std::max(std::exp(-a_i*l*l), std::exp(-a_i*u*u));
}


int rand_exp_range(Point lower, Point upper, NT a_i, Point &p, vars &var){
    NT r, r_val, fn;
    Point bef = upper-lower;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    RNGType rng(seed);
    RNGType &rng2 = var.rng;
    if(a_i>0.00000001 && std::sqrt(bef.squared_length()) >= (2.0/std::sqrt(2.0*a_i))){
        boost::normal_distribution<> rdist(0,1);
        Point a = -1.0*lower;
        Point b = (1.0/std::sqrt(bef.squared_length()))*bef;
        Point z = (a.dot(b)*b)+lower;
        NT low_bd = (lower[0]-z[0])/b[0];
        NT up_bd = (upper[0]-z[0])/b[0];
        while(true){
            r = rdist(rng2);
            r = r/std::sqrt(2.0*a_i);
            if(r>=low_bd && r<=up_bd){
                break;
            }
        }
        p=(r*b)+z;

    }else{
        boost::random::uniform_real_distribution<> urdist(0,1);
        NT M=get_max(lower, upper, a_i);
        while(true){
            r=urdist(rng2);
            Point pef = r*upper;
            p = ((1.0-r)*lower)+ pef;
            r_val = M*urdist(var.rng);
            fn = eval_exp(p,a_i);
            if(r_val<fn){
                break;
            }
        }
    }
    return 1;
}


int rand_exp_range_coord(NT l, NT u, NT a_i, NT &dis, vars &var){
    NT r, r_val, fn;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    RNGType rng(seed);
    RNGType &rng2 = var.rng;
    if(a_i>std::pow(10,-8.0) && u-l>=2.0/std::sqrt(2.0*a_i)){
        boost::normal_distribution<> rdist(0,1);
        while(true){
            r = rdist(rng2);
            r = r/std::sqrt(2.0*a_i);
            if(r>=l && r<=u){
                break;
            }
        }
        dis=r;

    }else{
        boost::random::uniform_real_distribution<> urdist(0,1);
        NT M = get_max_coord(l, u, a_i);
        while(true){
            r=urdist(rng2);
            dis = (1.0-r)*l+r*u;
            r_val = M*urdist(rng2);
            fn = std::exp(-a_i*dis*dis);
            if(r_val<fn){
                break;
            }
        }
    }

    return 1;
}


template <class T>
int gaussian_next_point(T &P,
                        Point &p,   // a point to start
                        Point &p_prev,
                        int &coord_prev,
                        int walk_len,
                        NT a_i,
                        std::vector<NT> &lamdas,
                        vars &var)
{
    int n=var.n;
    //RNGType &rng = var.rng;
    boost::random::uniform_int_distribution<> uidist(0,n-1);
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    RNGType rng(seed);
    RNGType &rng2 = var.rng;
    NT ball_rad;

    if(var.ball_walk) {
        ball_rad = 4.0*var.ball_radius/std::sqrt(std::max(1.0,a_i)*NT(n));
        gaussian_ball_walk(p, P, a_i, ball_rad, var);
    }else if(!var.coordinate){
        gaussian_hit_and_run(p,P,a_i,var);
    }else{
        int rand_coord;
        if(coord_prev==-1){
            p_prev = p;
            rand_coord = uidist(rng2);
            gaussian_hit_and_run_coord_update(p,p_prev,P,rand_coord,rand_coord,a_i,lamdas,var,true);
            coord_prev=rand_coord;
            if(walk_len==1){
                return 1;
            }else{
                walk_len--;
            }
        }
        for(int j=0; j<walk_len; j++){
            rand_coord = uidist(rng2);
            gaussian_hit_and_run_coord_update(p,p_prev,P,rand_coord,coord_prev,a_i,lamdas,var,false);
            coord_prev=rand_coord;
        }
    }

    return 1;
}


template <class T, class K>
int rand_gaussian_point_generator(T &P,
                         Point &p,   // a point to start
                         int rnum,
                         int walk_len,
                         K &randPoints,
                         NT a_i,
                         vars &var)  // constans for volume
{
    //std::cout<<"EDW2!"<<std::endl;
    int n = var.n;
    //bool birk = var.birk;
    RNGType &rng = var.rng;
    RNGType &rng2 = var.rng;
    //boost::random::uniform_real_distribution<> urdist(0,1);
    boost::random::uniform_int_distribution<> uidist(0,n-1);
    //std::uniform_real_distribution<NT> urdist = var.urdist;
    //std::uniform_int_distribution<int> uidist(0,n-1);

    std::vector<NT> lamdas(P.num_of_hyperplanes(),NT(0));
    //int rand_coord = rand()%n;
    //double kapa = double(rand())/double(RAND_MAX);
    int rand_coord = uidist(rng2);
    NT ball_rad;
    //double kapa = urdist(rng);
    Point p_prev = p;
    if(var.ball_walk) {
        ball_rad = 4.0*var.ball_radius/std::sqrt(std::max(1.0,a_i)*NT(n));
        gaussian_ball_walk(p, P, a_i, ball_rad, var);
        randPoints.push_back(p);
    }else if(var.coordinate){
        //std::cout<<"[1a]P dim: "<<P.dimension()<<std::endl;
        gaussian_hit_and_run_coord_update(p,p_prev,P,rand_coord,rand_coord,a_i,lamdas,var,true);
        randPoints.push_back(p);
        //std::cout<<"[1b]P dim: "<<P.dimension()<<std::endl;
    }else {
        gaussian_hit_and_run(p, P, a_i, var);
        randPoints.push_back(p);
    }

    for(int i=1; i<rnum; ++i){

        for(int j=0; j<walk_len; ++j){
            int rand_coord_prev = rand_coord;
            //rand_coord = rand()%n;
            //kapa = double(rand())/double(RAND_MAX);
            rand_coord = uidist(rng2);
            //kapa = urdist(rng);
            if(var.ball_walk) {
                gaussian_ball_walk(p, P, a_i, ball_rad, var);
            }else if(var.coordinate){
                //std::cout<<"[1c]P dim: "<<P.dimension()<<std::endl;
                gaussian_hit_and_run_coord_update(p,p_prev,P,rand_coord,rand_coord_prev,a_i,lamdas,var,false);
            }else
                gaussian_hit_and_run(p,P,a_i,var);
        }
        randPoints.push_back(p);
        //if(birk) birk_sym(P,randPoints,p);
    }
    return 1;

    //if(rand_only) std::cout<<p<<std::endl;
    //if(print) std::cout<<"("<<i<<") Random point: "<<p<<std::endl;
}


Point get_dir(vars var){
    int dim=var.n;
    boost::normal_distribution<> rdist(0,1);
    std::vector<NT> Xs(dim,0);
    NT normal=NT(0);
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    RNGType rng(seed);
    RNGType rng2 = var.rng;
    for (int i=0; i<dim; i++){
        Xs[i]=rdist(rng);
        //std::cout<<Xs[i]<<" ";
        normal+=Xs[i]*Xs[i];
    }
    //std::cout<<"\n";
    normal=1.0/std::sqrt(normal);

    for (int i=0; i<dim; i++){
        Xs[i]=Xs[i]*normal;
    }
    Point p(dim, Xs.begin(), Xs.end());
    return p;

}


Point get_point_in_Dsphere(vars var, NT radius){
    int dim = var.n;
    boost::normal_distribution<> rdist(0,1);
    boost::random::uniform_real_distribution<> urdist(0,1);
    std::vector<NT> Xs(dim,0);
    NT normal=NT(0), U;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    RNGType rng(seed);
    RNGType &rng2 = var.rng;
    for (int i=0; i<dim; i++){
        Xs[i]=rdist(rng2);
        //std::cout<<Xs[i]<<" ";
        normal+=Xs[i]*Xs[i];
    }
    //std::cout<<"\n";
    normal=1.0/std::sqrt(normal);

    for (int i=0; i<dim; i++){
        Xs[i]=Xs[i]*normal;
    }
    Point p(dim, Xs.begin(), Xs.end());
    U = urdist(rng2);
    U = std::pow(U, 1.0/(NT(dim)));
    p = (radius*U)*p;
    return p;
}


template <class T>
int gaussian_hit_and_run(Point &p,
                T &P,
                NT a_i,
                vars &var)
{
    int n = var.n;
    double err = var.err;
    RNGType &rng = var.rng;
    RNGType rng2 = var.rng;
    //std::uniform_real_distribution<NT> &urdist = var.urdist;
    //boost::random::uniform_real_distribution<> urdist(0,1);   //std::uniform_real_distribution<NT> &urdist1 = var.urdist1;

    //Point origin(n);

    //Random_points_on_sphere_d<Point> gen (n, 1.0);
    //Point l = gen.sample_point(var.rng);// - CGAL::Origin();
    Point l=get_dir(var);
    //Point l2=origin;
    //Vector b1 = line_bisect(p,l,P,var,var2);
    //Vector b2 = line_bisect(p,-l,P,var,var2);
    //std::pair<Point,Point> ppair = P.line_intersect(p,l);
    std::pair<NT,NT> dbpair = P.line_intersect(p,l);
    NT min_plus = dbpair.first;
    NT max_minus = dbpair.second;
    //NT dis;
    //rand_exp_range(max_minus, min_plus, a_i, dis, var);
    //if(var.verbose) std::cout<<"max_minus = "<<max_minus<<"min_plus = "<<min_plus<<std::endl;
    //p = (dis*l)+p;
    Point upper = (min_plus*l)+p;
    Point lower = (max_minus*l)+p;
    rand_exp_range(lower, upper, a_i, p, var);
    //Point b1 = ppair.first;// - origin;
    //Point b2 = ppair.second;// - origin;
    //std::cout<<"b1="<<b1<<"b2="<<b2<<std::endl;

    //NT lambda = urdist(rng);
    //p = (lambda*b1);
    //p=((1-lambda)*b2) + p;
    return 1;
}


//hit-and-run with orthogonal directions and update
template <class T>
int gaussian_hit_and_run_coord_update(Point &p,
                             Point &p_prev,
                             T &P,
                             int rand_coord,
                             int rand_coord_prev,
                             NT a_i,
                             std::vector<NT> &lamdas,
                             vars &var,
                             bool init)
{
    //std::cout<<"[1]P dim: "<<P.dimension()<<std::endl;
    std::pair<NT,NT> bpair;
    // EXPERIMENTAL
    //if(var.NN)
    //  bpair = P.query_dual(p,rand_coord);
    //else
    bpair = P.line_intersect_coord(p,p_prev,rand_coord,rand_coord_prev,lamdas,init);
    NT min_plus = bpair.first;
    NT max_minus = bpair.second;
    NT dis;
    rand_exp_range_coord(p[rand_coord]+max_minus, p[rand_coord]+min_plus, a_i, dis, var);
    p_prev = p;
    p.set_coord(rand_coord , dis);
    return 1;
}


//Ball walk. Test_2
template <class T>
int gaussian_ball_walk(Point &p,
              T &P,
              NT a_i,
              NT ball_rad,
              vars var)
{
    int n =P.dimension();
    //NT radius = var.ball_radius;
    NT f_x,f_y,rnd;
    Point y=get_point_in_Dsphere(var, ball_rad);
    y=y+p;
    f_x = eval_exp(p,a_i);
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    RNGType rng(seed);
    RNGType &rng2=var.rng;
    boost::random::uniform_real_distribution<> urdist(0,1);
    if (P.is_in(y)==-1){
        f_y = eval_exp(y,a_i);
        rnd = urdist(rng2);
        if(rnd <= f_y/f_x){
            p=y;
        }
    }
    return 1;
}




#endif