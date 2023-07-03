%Function level1b_exporter_matlab exports odin database data.
%The fucntion exports calibration data from one orbit and backend 
%and data is stored into a matlab structure.
%
%The function performs the following main tasks
%1. find out which spectra that belongs to scans that start
%within the orbit
%2. export all target and calibration spectra information from the database 
%and decode data into two array of structures
%3. combine the two arrays into a single with data in correct order 
%4. perform a second calibration step on data (if desired)   
%
% USAGE:  [L,ok]=level1b_exporter_matlab(orbit,backend,calibration_step2)
%
%    IN:      
%             orbit: orbit number 
%             backend: ('AC1' or 'AC2')
%             calibration_step2:  (0 or 1)
%             where 1 apply calibration_step2
%
%    OUT:     
%             L.spec_h: struct with fields of Odin spectrum information
%             L.y     : cell array holding spectra
%             ok: 0 or 1 (0=data is not available)
%
% EXAMPLE USAGE [L,ok]=level1b_exporter_matlab(46885,'AC1',1);        
%                
%L = 
%    spec_h: [1x1 struct]
%         y: {1x2245 cell}       
%
%L.spec_h=
%            Version: [1x2245 double]
%              Level: [1x2245 double]
%            Quality: [1x2245 double]
%                STW: [1x2245 double]
%                MJD: [1x2245 double]
%              Orbit: [1x2245 double]
%                LST: [1x2245 double]
%             Source: [32x2245 char]
%         Discipline: [1x2245 double]
%              Topic: [1x2245 double]
%           Spectrum: [1x2245 double]
%            ObsMode: [1x2245 double]
%               Type: [1x2245 double]
%           Frontend: [1x2245 double]
%            Backend: [1x2245 double]
%         SkyBeamHit: [1x2245 double]
%             RA2000: [1x2245 double]
%            Dec2000: [1x2245 double]
%            VSource: [1x2245 double]
%          Longitude: [1x2245 double]
%           Latitude: [1x2245 double]
%           Altitude: [1x2245 double]
%            Qtarget: [4x2245 double]
%          Qachieved: [4x2245 double]
%             Qerror: [3x2245 double]
%             GPSpos: [3x2245 double]
%            GPSvel: [3x2245 double]
%             SunPos: [3x2245 double]
%            MoonPos: [3x2245 double]
%              SunZD: [1x2245 double]
%               Vgeo: [1x2245 double]
%               Vlsr: [1x2245 double]
%               Tcal: [1x2245 double]
%               Tsys: [1x2245 double]
%             SBpath: [1x2245 double]
%             LOFreq: [1x2245 double]
%            SkyFreq: [1x2245 double]
%           RestFreq: [1x2245 double]
%     MaxSuppression: [1x2245 double]
%    AttitudeVersion: [1x2245 double]
%            FreqRes: [1x2245 double]
%            FreqCal: [4x2245 double]
%            IntMode: [1x2245 double]
%            IntTime: [1x2245 double]
%            EffTime: [1x2245 double]
%           Channels: [1x2245 double]
%           FreqMode: [1x2245 double]
%             TSpill: [1x2245 double]
%             ScanID: [1x2245 double]
%
%spectrum1=L.y{1} 
%size(spectrum1)
%  896 1
%
%--Handling of scan
% 
%scanid=unique( unique(L.spec_h.ScanID));
%--find index for first scan
%scan1_index=find(L.spec_h.ScanID==scanid(1));
%--write spectra from scan 1 into matrix spectra_scan1
%spectra_scan1=[L.y{scan1_index}];
%size(spectra_scan1)=
%  896    35



function [spectra,ok]=level1b_exporter_matlab(orbit,backend,calibration_step2)

%wget http://jdbc.postgresql.org/download/postgresql-9.2-1002.jdbc4.jar
% Add jar file to classpath 
javaaddpath('/home/odinop/odincal_tmp/test/postgresql-9.2-1002.jdbc4.jar')


%open a connection to the odin database
passwd = getenv('ODINDB_PASSWD')
conn = database('odin','odinop',passwd,'Vendor',...
		'PostGreSQL','Server','malachite'); 

%get min and max stw from all scans that start in the orbit
[minstw,maxstw]=get_orbit_stw(orbit,backend,conn);
if isnan(minstw);
   spectra=[];
   ok=0;
   'data not available'
   return
end

%get target spectra data from the orbit
[Target_spec,ok]=export_orbit_target_data(orbit,backend,conn,minstw,maxstw);
if ok==0;
   spectra=[];
   ok=0;
   'data not available'
   return
end

%get calibration spectra data from the orbit
[Calibration_spec,ok]=export_orbit_cal_data(orbit,backend,conn,minstw,maxstw);
if ok==0;
   spectra=[];
   ok=0;
   'data not available'
   return
end


%combine target and cal data in correct order
[spectra]=combine_target_and_cal_data(Target_spec,Calibration_spec);


if calibration_step2==1;
   %perform calibration step2
   %create an empty structure for calibration step2
   cal_spectrum_id=struct('backend',[],...
                          'frontend',[],...
                          'version',[],...
                          'intmode',[],...
                          'sourcemode',[],...
                          'freqmode',[],...
                          'ssb_fq',[],...
                          'altitude_range',[],...
                          'hotload_range',[]);
   cal_spectrum=struct('id',cal_spectrum_id,...
                    'median_fit',[]);

   cal_spectra{1}=cal_spectrum;

   for i=1:length(spectra);
       if spectra(i).Type=='SPE';
          %export a new cal_spectrum structure (if necessary)
          [cal_spectrum,cal_spectra]=...
          export_calibration_step2_data(spectra,i,cal_spectra,conn);
          %apply calibration step2
          spectra=cal_step2(spectra,i,cal_spectrum);
       end
   end
end

%close connection
close(conn)

%transform structure of data
[spectra]=convert_structure(spectra);


ok=1;
end
%---end of main function-------------------------------------------------------

%-------------------------------------------------------------------------------
%sub-functions
%

function [minstw,maxstw]=get_orbit_stw(orbit,backend,conn)


% find min and max stws from orbit
sql=sprintf(['select min(foo.stw),max(foo.stw) from ', ...
	     '(select stw from attitude_level1 where ',...
	     'orbit>=%d and orbit<%d order by stw) as foo'],orbit,orbit+1);  
                                
% Get all records
rs=fetch(conn,sql);
minstw=rs{1};
maxstw=rs{2};
if isnan(minstw);
   return
end

%find out min and max stw from scans that starts in the orbit
if backend=='AC1';
 stwoff=1;
else;
 stwoff=0;
end

if backend=='AC1';
  sql=sprintf(['select min(ac_level0.stw),max(ac_level0.stw) ',...
	       'from ac_level0 ',...
	       'natural join getscansac1(%d,%d+16*60*45) ',... 
	       'where start>=%d and start<=%d ',...
	       'and backend=''%s'' '],minstw,maxstw,minstw,maxstw,backend);
end		
if backend=='AC2';
  sql=sprintf(['select min(ac_level0.stw),max(ac_level0.stw) ',...
	       'from ac_level0 ',...
	       'natural join getscansac2(%d,%d+16*60*45) ',... 
	       'where start>=%d and start<=%d ',...
	       'and backend=''%s'' '],minstw,maxstw,minstw,maxstw,backend);  
end
    
rs=fetch(conn,sql); 
minstw=rs{1};
maxstw=rs{2}; 

end
%end of function get_orbit_stw
%------------------------------------------------------------------------------ 

function [Target_spec,ok]=export_orbit_target_data(orbit,backend,conn,minstw,maxstw)
%export data from database
%
%export all target spectra data from the orbit
%
sql=sprintf(['select version,alevel,stw,mjd, ',...
	     'orbit,lst,sourcemode,freqmode, ',...
             'ac_level1b.frontend,backend,skybeamhit, ',...
	     'ra2000,dec2000,vsource,latitude,longitude,altitude, ',...
	     'qtarget,qachieved,qerror,gpspos,gpsvel,sunpos,moonpos, ',...
	     'sunzd,vgeo,vlsr,hotloada,hotloadb,tsys,sbpath, ',...
	     'lofreq,skyfreq,restfreq, ',...
             'maxsuppression,ac_level1b.soda,ssb_fq, ',...
	     'intmode,inttime,efftime,channels,spectra,calstw,sig_type ',...
	     'from ac_level1b ',...
	     'join attitude_level1  using (backend,stw) ',...
             'join ac_level0  using (backend,stw) ',...
             'join shk_level1  using (backend,stw) ',...
             'where stw>=%d and stw<=%d and backend=''%s'' ',...
             'and version=8 and sig_type=''SIG'' ',...
	     'order by stw asc,intmode asc '],minstw,maxstw,backend);
rs1=fetch(conn,sql);
if length(rs1)==0;
   Target_spec=[];
   ok=0;
   return
else;
   ok=1;
end

%now decode the target spectra from database data

for i=1:length(rs1);
   header={rs1{i,:}};
 
   if isequal(char(header{10}),'AC1');
      backend=1;
   elseif isequal(char(header{10}),'AC2');
      backend=2;
   end
  
   if isequal(char(header{9}),'555');
      frontend=1;
   elseif isequal(char(header{9}),'495');
      frontend=2;
   elseif isequal(char(header{9}),'572');
      frontend=3;
   elseif isequal(char(header{9}),'549');
      frontend=4;
   elseif isequal(char(header{9}),'119');
      frontend=5;
   elseif isequal(char(header{9}),'SPL');
      frontend=6;
   end
  

   source = strrep( char(header{7}),'ODD_H','Odd hydrogen');
   source = strrep( source,'ODD_N','Odd nitrogen');
   source = strrep( source, 'WATER', 'Water isotope');
   source = strrep( source,'SUMMER','Summer mesosphere');
   source = strrep( source,'STRAT','Stratospheric');
   source = strrep( source,'DYNAM','Transport');
   source = sprintf([source,' FM=%d'],header{8});
   source = sprintf('%-*s',32,source);
   tcal=double(header{28});
   if tcal==0;
	tcal=double(header{29});
   end
   spec_h = struct('Version',double(header{1}),...
                   'Level',double(header{2}),...
                   'Quality',double(header{1}),...
                   'STW',double(header{3}),...
                   'MJD',double(header{4}),...
                   'Orbit',double(header{5}),...
                   'LST',double(header{6}),...
                   'Source',source,...
                   'Discipline',1,...
                   'Topic',1,...
                   'Spectrum',double(i),...
                   'ObsMode',2,...
                   'Type',8,...
                   'Frontend',frontend,...
                   'Backend',backend,...
                   'SkyBeamHit',double(header{11}),...
                   'RA2000',double(header{12}),...
                   'Dec2000',double(header{13}),...
                   'VSource',double(header{14}),...
                   'Latitude',double(header{15}),...
                   'Longitude',double(header{16}),...
                   'Altitude',double(header{17}),...
                   'Qtarget',header{18},...
                   'Qachieved',header{19},...
                   'Qerror',header{20},...
                   'GPSpos',header{21},...
                   'GPSvel',header{22},...
                   'SunPos',header{23},...
                   'MoonPos',header{24},...
                   'SunZD',double(header{25}),...
                   'Vgeo',double(header{26}),...
                   'Vlsr',double(header{27}),...
                   'Tcal',tcal,...
                   'Tsys',double(header{30}),...
                   'SBpath',double(header{31}),...
                   'LOFreq',double(header{32}),...
                   'SkyFreq',double(header{33}),...
                   'RestFreq',double(header{34}),...
                   'MaxSuppression',double(header{35}),...
                   'SodaVersion',double(header{36}),...
                   'FreqRes',double(1000000),...
                   'FreqCal',header{37},...
                   'IntMode',double(header{38}),...
                   'IntTime',double(header{39}),...
                   'EffTime',double(header{40}),...
                   'Channels',double(header{41}),...
                   'Tb',typecast(header{42},'double'),...
                   'Tspill',0,...
		   'CALSTW',double(header{43}),...
		   'FreqMode',double(header{8}));

   spec_h.Qtarget=double(spec_h.Qtarget.getArray());
   spec_h.Qachieved=double(spec_h.Qachieved.getArray());
   spec_h.Qerror=double(spec_h.Qerror.getArray());
   spec_h.GPSpos=double(spec_h.GPSpos.getArray());
   spec_h.GPSvel=double(spec_h.GPSvel.getArray());
   spec_h.SunPos=double(spec_h.SunPos.getArray());
   spec_h.MoonPos=double(spec_h.MoonPos.getArray());
   spec_h.FreqCal=double(spec_h.FreqCal.getArray());
   Target_spec{i}=spec_h;
end


end
%-end of function export_orbit_target_data
%-----------------------------------------------------------------------------

function [Calibration_spec,ok]=export_orbit_cal_data(orbit,backend,conn,minstw,maxstw)
%
%export all calibration spectra data from the orbit
sql=sprintf(['select version,alevel,stw,mjd, ',...
	     'orbit,lst,sourcemode,freqmode, ',...
             'ac_cal_level1b.frontend,backend,skybeamhit, ',...
	     'ra2000,dec2000,vsource,latitude,longitude,altitude, ',...
	     'qtarget,qachieved,qerror,gpspos,gpsvel,sunpos,moonpos, ',...
	     'sunzd,vgeo,vlsr,hotloada,hotloadb,sbpath, ',...
	     'lofreq,skyfreq,restfreq, ',...
             'maxsuppression,ac_cal_level1b.soda,ssb_fq, ',...
	     'intmode,inttime,channels,spectra,spectype,tspill ',...
	     'from ac_cal_level1b ',...
	     'join attitude_level1  using (backend,stw) ',...
             'join ac_level0  using (backend,stw) ',...
             'join shk_level1  using (backend,stw) ',...
             'where stw>=%d and stw<=%d and backend=''%s'' ',...
             'and version=8 ',...
	     'order by stw asc,intmode asc '],minstw,maxstw,backend);

rs2=fetch(conn,sql);

if length(rs2)==0;
   Calibration_spec=[];
   ok=0;
   return
else;   
   ok=1;
end

%now decode the calibration spectra from database data
Calibration_spec={};
for i=1:length(rs2);
  
   header={rs2{i,:}};

   if isequal(char(header{10}),'AC1');
      backend=1;
   elseif isequal(char(header{10}),'AC2');
      backend=2;
   end
  
   if isequal(char(header{9}),'555');
      frontend=1;
   elseif isequal(char(header{9}),'495');
      frontend=2;
   elseif isequal(char(header{9}),'572');
      frontend=3;
   elseif isequal(char(header{9}),'549');
      frontend=4;
   elseif isequal(char(header{9}),'119');
      frontend=5;
   elseif isequal(char(header{9}),'SPL');
      frontend=6;
   end
  
   if isequal(char(header{41}),'CAL');
      sigtype=3;
   elseif isequal(char(header{41}),'SSB');
      sigtype=9;
   end
 
   source = strrep( char(header{7}),'ODD_H','Odd hydrogen');
   source = strrep( source,'ODD_N','Odd nitrogen');
   source = strrep( source, 'WATER', 'Water isotope');
   source = strrep( source,'SUMMER','Summer mesosphere');
   source = strrep( source,'STRAT','Stratospheric');
   source = strrep( source,'DYNAM','Transport');
   source = sprintf([source,' FM=%d'],header{8});
   source = sprintf('%-*s',32,source);
   spec_h = struct('Version',double(header{1}),...
                   'Level',double(header{2}),...
                   'Quality',double(header{1}),...
                   'STW',double(header{3}),...
                   'MJD',double(header{4}),...
                   'Orbit',double(header{5}),...
                   'LST',double(header{6}),...
                   'Source',char(source),...
                   'Discipline',1,...
                   'Topic',1,...
                   'Spectrum',double(i),...
                   'ObsMode',2,...
                   'Type',sigtype,...
                   'Frontend',frontend,...
                   'Backend',backend,...
                   'SkyBeamHit',double(header{11}),...
                   'RA2000',double(header{12}),...
                   'Dec2000',double(header{13}),...
                   'VSource',double(header{14}),...
                   'Latitude',double(header{15}),...
                   'Longitude',double(header{16}),...
                   'Altitude',double(header{17}),...
                   'Qtarget',header{18},...
                   'Qachieved',header{19},...
                   'Qerror',header{20},...
                   'GPSpos',header{21},...
                   'GPSvel',header{22},...
                   'SunPos',header{23},...
                   'MoonPos',header{24},...
                   'SunZD',double(header{25}),...
                   'Vgeo',double(header{26}),...
                   'Vlsr',double(header{27}),...
                   'Tcal',double(header{28}),...
                   'Tsys',double(0),...
                   'SBpath',double(header{30}),...
                   'LOFreq',double(header{31}),...
                   'SkyFreq',double(header{32}),...
                   'RestFreq',double(header{33}),...
                   'MaxSuppression',double(header{34}),...
                   'SodaVersion',double(header{35}),...
                   'FreqRes',double(1000000),...
                   'FreqCal',header{36},...
                   'IntMode',double(header{37}),...
                   'IntTime',double(header{38}),...
                   'EffTime',double(0),...
                   'Channels',double(header{39}),...
                   'Tb',typecast(header{40},'double'),...
                   'Tspill',double(header{42}),...
                   'CALSTW',double(header{3}),...
		   'FreqMode',double(header{8}));

   spec_h.Qtarget=double(spec_h.Qtarget.getArray());
   spec_h.Qachieved=double(spec_h.Qachieved.getArray());
   spec_h.Qerror=double(spec_h.Qerror.getArray());
   spec_h.GPSpos=double(spec_h.GPSpos.getArray());
   spec_h.GPSvel=double(spec_h.GPSvel.getArray());
   spec_h.SunPos=double(spec_h.SunPos.getArray());
   spec_h.MoonPos=double(spec_h.MoonPos.getArray());
   spec_h.FreqCal=double(spec_h.FreqCal.getArray());
   Calibration_spec{i}=spec_h;
end


end
%-end of function export_orbit_cal_data
%-----------------------------------------------------------------------------

function [spectra]=combine_target_and_cal_data(Target_spec,Calibration_spec)
%now we have two structures (Target_spec and Calibration_spec) of data
%combine data in correct order
%
%first add calibration spectra from a scan
%and then all target spectrum for the scan
%into the structure spectra 

n=1;
spectra={};
for i=1:length(Calibration_spec);
  %add calibration spectrum
  spectra{n}=Calibration_spec{i};
  spectra{n}.Spectrum=n;
  n=n+1;
  if i<length(Calibration_spec);
    if Calibration_spec{i}.STW==Calibration_spec{i+1}.STW;
       continue
    end
  end
  %add target data for correct scan
  for j=1:length(Target_spec);
    if Calibration_spec{i}.STW==Target_spec{j}.CALSTW;
       spectra{n}=Target_spec{j};
       spectra{n}.Spectrum=n;
       spectra{n}.Tspill=Calibration_spec{i}.Tspill;
       n=n+1;
    end
  end
end

%now spectra from cell array to struct array
spectra = cell2mat(spectra);

end
%-end of function combine_target_and_cal_data
%-----------------------------------------------------------------------------

function [cal_spectrum_temp,cal_spectra]=export_calibration_step2_data(spectra,i,cal_spectra,conn);
%load data to be used for calibration step2

cal_spectrum_temp_id=cal_spectra{1}.id; %an empty structure
%now fill the structure for this spectrum
cal_spectrum_temp_id.backend=spectra(i).Backend;
cal_spectrum_temp_id.frontend=spectra(i).Frontend;
cal_spectrum_temp_id.version=spectra(i).Version;
cal_spectrum_temp_id.intmode=spectra(i).IntMode;
n = regexp(spectra(i).Source,...
	   '(?<sourcemode>.*) FM=(?<freqmode>\d+)$','names');
source = strrep( n.sourcemode,'Odd hydrogen','ODD_H');
source = strrep( source,'Odd nitrogen','ODD_N');
source = strrep( source, 'Water isotope','WATER');
source = strrep( source,'Summer mesosphere','SUMMER');
source = strrep( source,'Stratospheric','STRAT');
source = strrep( source,'Transport','DYNAM');
cal_spectrum_temp_id.sourcemode=source;
cal_spectrum_temp_id.freqmode=str2num(n.freqmode);
cal_spectrum_temp_id.ssb_fq=sprintf('{%d,%d,%d,%d}',spectra(i).FreqCal(1),...
    spectra(i).FreqCal(2),spectra(i).FreqCal(3),spectra(i).FreqCal(4));   
cal_spectrum_temp_id.altitude_range='{80000,120000}';
cal_spectrum_temp_id.hotload_range=sprintf('{%d,%d}',floor(spectra(i).Tcal),...
					ceil(spectra(i).Tcal));

%check if we already have exported necessary data

for j=1:length(cal_spectra);
    if isequal(cal_spectrum_temp_id,cal_spectra{j}.id);
       %we have already data
       cal_spectrum_temp=cal_spectra{j};
       return
    end
end  

%now we need to export data from the database

sql=sprintf(['select hotload_range,median_fit,channels ',...
	     'from ac_cal_level1c where backend=''%s'' and ',...
	     'frontend=''%s'' and version=%d and intmode=%d ',...
	     'and sourcemode=''%s'' and freqmode=%d and ',...
	     'ssb_fq=''%s'' and altitude_range=''%s'' and ',...
	     'hotload_range=''%s'' '],...
              cal_spectrum_temp_id.backend,...
              cal_spectrum_temp_id.frontend,...
	      cal_spectrum_temp_id.version,...
              cal_spectrum_temp_id.intmode,...
	      cal_spectrum_temp_id.sourcemode,...
              cal_spectrum_temp_id.freqmode,...
              cal_spectrum_temp_id.ssb_fq,...
              cal_spectrum_temp_id.altitude_range,...
	      cal_spectrum_temp_id.hotload_range);
    
rs3=fetch(conn,sql);
if length(rs3)>0;
   cal_spectrum_temp.median_fit=typecast(rs3{2},'double');
   cal_spectrum_temp.got_data=1;
else;
   cal_spectrum_temp.median_fit=0;
   cal_spectrum_temp.got_data=0;
end
cal_spectrum_temp.id=cal_spectrum_temp_id;
k=length(cal_spectra);
cal_spectra{k+1}=cal_spectrum_temp;

end
%end of export_calibration_step2_data
%-----------------------------------------------------------------------------

function [spectra]=cal_step2(spectra,i,cal_spectrum)
%apply calibration step2
Tcal=spectra(i).Tcal;
SkyFreq=spectra(i).SkyFreq;
t_load=planck(Tcal,SkyFreq);
t_sky=planck(2.7,SkyFreq);
eta=1-spectra(i).Tspill/300.0;%main beam efficeiency
w=1/eta*(1- ( spectra(i).Tb )/ ( t_load ));
spectra(i).Tb=spectra(i).Tb-w.*cal_spectrum.median_fit;

end
%end of function calibration_step2
%-----------------------------------------------------------------------------
function [Tb]=planck(T,f)
h = 6.626176e-34;     %Planck constant (Js)
k = 1.380662e-23;     %Boltzmann constant (J/K)
T0 = h*f/k;
if (T > 0.0); 
  Tb = T0/(exp(T0/T)-1.0);
else;         
  Tb = 0.0;
end

end
%end of function planck
%-----------------------------------------------------------------------------
function [C]=convert_structure(spectra)

B.Version=[spectra(:).Version];
B.Level=[spectra(:).Level];
B.Quality=[spectra(:).Quality];
B.STW=[spectra(:).STW];
B.MJD=[spectra(:).MJD];
B.Orbit=[spectra(:).Orbit];
B.LST=[spectra(:).LST];
B.Source=char(spectra(:).Source)';
B.Discipline=[spectra(:).Discipline];
B.Topic=[spectra(:).Topic];
B.Spectrum=[spectra(:).Spectrum];
B.ObsMode=[spectra(:).ObsMode];
B.Type=[spectra(:).Type];
B.Frontend=[spectra(:).Frontend];
B.Backend=[spectra(:).Backend];
B.SkyBeamHit=[spectra(:).SkyBeamHit];
B.RA2000=[spectra(:).RA2000];
B.Dec2000=[spectra(:).Dec2000];
B.VSource=[spectra(:).VSource];
B.Longitude=[spectra(:).Longitude];
B.Latitude=[spectra(:).Latitude];
B.Altitude=[spectra(:).Altitude];
B.Qtarget=[spectra(:).Qtarget];
B.Qachieved=[spectra(:).Qachieved];
B.Qerror=[spectra(:).Qerror];
B.GPSpos=[spectra(:).GPSpos];
B.GPSvel=[spectra(:).GPSvel];
B.SunPos=[spectra(:).SunPos];
B.MoonPos=[spectra(:).MoonPos];
B.SunZD=[spectra(:).SunZD];
B.Vgeo=[spectra(:).Vgeo];
B.Vlsr=[spectra(:).Vlsr];
B.Tcal=[spectra(:).Tcal];
B.Tsys=[spectra(:).Tsys];
B.SBpath=[spectra(:).SBpath];
B.LOFreq=[spectra(:).LOFreq];
B.SkyFreq=[spectra(:).SkyFreq];
B.RestFreq=[spectra(:).RestFreq];
B.MaxSuppression=[spectra(:).MaxSuppression];
B.AttitudeVersion=[spectra(:).SodaVersion];
B.FreqRes=[spectra(:).FreqRes];
B.FreqCal=[spectra(:).FreqCal];
B.IntMode=[spectra(:).IntMode];
B.IntTime=[spectra(:).IntTime];
B.EffTime=[spectra(:).EffTime];
B.Channels=[spectra(:).Channels];
%B.pointer=[spectra(:).pointer];
B.FreqMode=[spectra(:).FreqMode];
B.TSpill=[spectra(:).Tspill];
%B.Tb=[spectra(:).Tb];
B.ScanID=[spectra(:).CALSTW];


Tb={spectra(:).Tb};

C.spec_h=B;
C.y=Tb;

end
%end of function convert_structure
%-----------------------------------------------------------------------------












               


