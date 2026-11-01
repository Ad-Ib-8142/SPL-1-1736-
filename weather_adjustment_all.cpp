// Dhaka GSI: one-file, C++11, Windows MinGW-compatible weather adjustment.
// Run from the spl1 project root. Originals are never modified.
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cerrno>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
using namespace std;

struct Site { double lat, lon; };
static const Site sites[30] = {
 {23.895,90.405},{23.890,90.440},{23.885,90.475},{23.875,90.365},
 {23.870,90.405},{23.870,90.450},{23.865,90.490},{23.855,90.375},
 {23.850,90.415},{23.850,90.460},{23.845,90.495},{23.835,90.355},
 {23.830,90.395},{23.830,90.435},{23.825,90.475},{23.810,90.365},
 {23.810,90.405},{23.810,90.445},{23.805,90.485},{23.790,90.375},
 {23.785,90.415},{23.785,90.455},{23.770,90.390},{23.765,90.430},
 {23.760,90.470},{23.745,90.385},{23.740,90.425},{23.730,90.455},
 {23.715,90.405},{23.700,90.440}
};
struct Dataset { const char* group; const char* name; };
static const Dataset inputs[] = {
 {"rainfall","rain_DNCC_DSCC.csv"},
 {"rainfall","precipitation_from_humidity_filename_DNCC_DSCC.csv"},
 {"temperature","temperature_2m_DNCC_DSCC.csv"},
 {"meteorology","wind_DNCC_DSCC.csv"},
 {"meteorology","cloudcovertotal_DNCC_DSCC.csv"},
 {"meteorology","evaporation_DNCC_DSCC.csv"},
 {"meteorology","surface_pressure_DNCC_DSCC.csv"},
 {"soil","soil_moisture_DNCC_DSCC.csv"},
 {"soil","soil_temperature_DNCC_DSCC.csv"}
};

static vector<string> csv(const string& s) {
 vector<string> v; string x; bool quote=false;
 for(size_t i=0;i<s.size();++i){
  if(s[i]=='"') { if(quote && i+1<s.size() && s[i+1]=='"'){x+='"';++i;} else quote=!quote; }
  else if(s[i]==',' && !quote){v.push_back(x);x.clear();}
  else x+=s[i];
 }
 v.push_back(x);return v;
}
static string esc(const string& s){
 if(s.find_first_of(",\"\n\r")==string::npos) return s;
 string q="\""; for(size_t i=0;i<s.size();++i){if(s[i]=='"')q+='"';q+=s[i];} return q+'"';
}
static bool number(const string& s,double& x){
 if(s.empty()) return false;
 char* e=0;
 x=strtod(s.c_str(),&e);
 return e!=s.c_str()&&*e=='\0'&&std::isfinite(x);
}
static bool integer(const string& s,int& x){
 if(s.empty())return false;
 char* e=0;
 long n=strtol(s.c_str(),&e,10);
 if(e==s.c_str()||*e!='\0'||n<0||n>1000000)return false;
 x=(int)n;return true;
}
static int dim(int y,int m){
 static const int d[12]={31,28,31,30,31,30,31,31,30,31,30,31};
 if(m<1||m>12)return 0;
 return m==2&&(y%400==0||(y%4==0&&y%100!=0))?29:d[m-1];
}
static bool bdt(const string& s,string& result){
 // Cleaned archive timestamps are UTC (Open-Meteo's default timezone for original request).
 if(s.size()!=16 && s.size()!=19)return false;
 if(s[4]!='-'||s[7]!='-'||(s[10]!='T'&&s[10]!=' ')||s[13]!=':')return false;
 if(s.size()==19&&s[16]!=':')return false;
 const int spans[]={0,5,8,11,14};const int sizes[]={4,2,2,2,2};
 int v[5]={0,0,0,0,0};
 for(int j=0;j<5;++j)for(int k=0;k<sizes[j];++k){char c=s[spans[j]+k];if(c<'0'||c>'9')return false;v[j]=v[j]*10+c-'0';}
 int y=v[0],m=v[1],d=v[2],h=v[3],minute=v[4];
 if(y<1900||d<1||d>dim(y,m)||h>23||minute>59)return false;
 int sec=0;
 if(s.size()==19){for(int k=17;k<19;++k){if(s[k]<'0'||s[k]>'9')return false;sec=sec*10+s[k]-'0';}if(sec>59)return false;}
 h+=6;if(h>=24){h-=24;++d;if(d>dim(y,m)){d=1;++m;if(m==13){m=1;++y;}}}
 ostringstream o;o<<setfill('0')<<setw(4)<<y<<'-'<<setw(2)<<m<<'-'<<setw(2)<<d<<'T'<<setw(2)<<h<<':'<<setw(2)<<minute;
 if(s.size()==19)o<<':'<<setw(2)<<sec;
 result=o.str();return true;
}
static double km(double a,double b,double c,double d){
 const double r=3.14159265358979323846/180.0;
 double x=sin((c-a)*r/2),y=sin((d-b)*r/2);
 double h=x*x+cos(a*r)*cos(c*r)*y*y;
 return 12742.0*asin(sqrt(std::max(0.0,std::min(1.0,h))));
}
static bool dir(const string& path){
#ifdef _WIN32
 return _mkdir(path.c_str())==0 || errno==EEXIST;
#else
 return mkdir(path.c_str(),0755)==0 || errno==EEXIST;
#endif
}
static string join(const string& a,const string& b){return a+"/"+b;}
struct Grid {int id;double lat,lon;};
static bool process(const string& root,const string& out,const Dataset& ds){
 string path=join(join(root,ds.group),ds.name);
 ifstream in(path.c_str());if(!in){cerr<<"MISSING: "<<path<<"\n";return false;}
 string line;if(!getline(in,line)){cerr<<"EMPTY: "<<path<<"\n";return false;}
 vector<string> header=csv(line);if(!header.empty()&&header[0].size()>=3&&
 (unsigned char)header[0][0]==239)header[0].erase(0,3);
 int ix=-1,it=-1,ilat=-1,ilon=-1,ielev=-1;
 for(size_t i=0;i<header.size();++i){
  if(header[i]=="location_id")ix=(int)i;
  if(header[i]=="time")it=(int)i;
  if(header[i]=="latitude")ilat=(int)i;
  if(header[i]=="longitude")ilon=(int)i;
  if(header[i]=="elevation")ielev=(int)i;
 }
 if(ix<0||it<0||ilat<0||ilon<0){cerr<<"UNSUPPORTED CSV HEADER: "<<path<<"\n";return false;}
 // First pass: discover each returned model-grid coordinate and one canonical source ID per grid.
 map<int,Site> origins;long long sourceRows=0;
 while(getline(in,line)){
  vector<string> row=csv(line);if(row.size()<header.size())continue;
  int id;double lat,lon;
  if(!integer(row[ix],id)||!number(row[ilat],lat)||!number(row[ilon],lon))continue;
  if(!origins.count(id))origins[id]=Site{lat,lon};
  ++sourceRows;
 }
 if(origins.empty()){cerr<<"NO VALID SOURCE LOCATIONS: "<<path<<"\n";return false;}
 vector<Grid> grids;
 for(map<int,Site>::const_iterator p=origins.begin();p!=origins.end();++p){
  bool exists=false;
  for(size_t g=0;g<grids.size();++g){
   if(fabs(grids[g].lat-p->second.lat)<0.000001&&fabs(grids[g].lon-p->second.lon)<0.000001){exists=true;break;}
  }
  if(!exists)grids.push_back(Grid{p->first,p->second.lat,p->second.lon});
 }
 string name=ds.name;name=name.substr(0,name.size()-4);
 string dataPath=join(out,name+"_30.csv");
 string mapPath=join(out,name+"_mapping.csv");
 ofstream data(dataPath.c_str()),mapout(mapPath.c_str());
 if(!data||!mapout){cerr<<"OUTPUT ERROR: "<<dataPath<<"\n";return false;}
 // Preserve every original weather variable and its original units/header.
 data<<"target_id,target_latitude,target_longitude,source_location_id,source_grid_latitude,source_grid_longitude,time_utc,time_bdt";
 vector<int> vars;
 for(size_t j=0;j<header.size();++j){
  if((int)j!=ix&&(int)j!=it&&(int)j!=ilat&&(int)j!=ilon&&(int)j!=ielev){vars.push_back((int)j);data<<","<<esc(header[j]);}
 }
 data<<"\n";
 mapout<<"target_id,target_latitude,target_longitude,source_location_id,source_grid_latitude,source_grid_longitude,distance_km,method\n";
 map<int,vector<int> > byId;
 for(int t=0;t<30;++t){
  double best=1e100;size_t chosen=0;
  for(size_t g=0;g<grids.size();++g){double d=km(sites[t].lat,sites[t].lon,grids[g].lat,grids[g].lon);
   if(d<best){best=d;chosen=g;}}
  const Grid& g=grids[chosen];byId[g.id].push_back(t);
  mapout<<t<<','<<fixed<<setprecision(6)<<sites[t].lat<<','<<sites[t].lon<<','<<g.id<<','<<g.lat<<','<<g.lon<<','<<setprecision(3)<<best<<",NEAREST_MODEL_GRID_COPY\n";
 }
 in.clear();in.seekg(0);getline(in,line);
 long long outputRows=0,bad=0;
 while(getline(in,line)){
  vector<string> row=csv(line);if(row.size()<header.size()){++bad;continue;}
  int id;if(!integer(row[ix],id)){++bad;continue;}
  map<int,vector<int> >::const_iterator selected=byId.find(id);
  if(selected==byId.end())continue; // alternate source IDs that share a selected grid
  string local;if(!bdt(row[it],local)){++bad;continue;}
  const Site& src=origins[id];
  string suffix;
  for(size_t j=0;j<vars.size();++j)suffix+=","+esc(row[vars[j]]);
  for(size_t k=0;k<selected->second.size();++k){
   int t=selected->second[k];
   data<<t<<','<<fixed<<setprecision(6)<<sites[t].lat<<','<<sites[t].lon<<','<<id<<','<<src.lat<<','<<src.lon<<','<<esc(row[it])<<','<<local<<suffix<<'\n';
   ++outputRows;
  }
 }
 if(!data.good()){cerr<<"WRITE FAILED: "<<dataPath<<"\n";return false;}
 cout<<"OK "<<ds.name<<" | source IDs: "<<origins.size()<<" | distinct grids: "<<grids.size()
     <<" | source rows: "<<sourceRows<<" | target rows: "<<outputRows<<" | skipped bad: "<<bad<<"\n";
 return true;
}
int main(int argc,char**argv){
 const string root=(argc>1?argv[1]:"data/datasets");
 const string out=(argc>2?argv[2]:"output/weather_adjusted");
 // Create folders; use paths relative to the current project directory.
 dir("output");if(!dir(out)){cerr<<"Cannot create output folder: "<<out<<"\n";return 1;}
 int done=0;const int count=(int)(sizeof(inputs)/sizeof(inputs[0]));
 for(int i=0;i<count;++i)if(process(root,out,inputs[i]))++done;
 cout<<"\nFinished: "<<done<<"/"<<count<<" weather datasets. Originals unchanged.\n";
 cout<<"NOTE: Copied nearest-grid values are NOT 30 independent weather measurements.\n";
 return done==count?0:1;
}
