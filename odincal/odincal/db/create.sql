
create type backend as enum ('AC1','AC2');
create type signal_type as enum ('REF','SIG');
create type frontend as enum ('549','495','572','555','SPL','119');
create type mech as enum ('REF','SK1','CAL','SK2');
create type scan as (start bigint, stw bigint,mech_type mech);
create type shk_type as enum ('LO495','LO549','LO555','LO572','SSB495','SSB549','SSB555','SSB572','mixC495','mixC549','mixC555','mixC572','imageloadA','imageloadB','hotloadA','hotloadB','mixerA','mixerB','lnaA','lnaB','119mixerA','119mixerB','warmifA','warmifB');

create type sourcemode as enum ('STRAT','ODD_H','ODD_N','WATER','SUMMER',
'DYNAM','N/A');

create type spectype as enum ('SIG','REF','CAL','CMB','DRK','SK1','SK2','SPE',
'SSB','AVE');

create table in_process(
   file varchar,
   version int,
   created timestamp,
   constraint pk_in_process_data primary key (file)
);

create table processed(
   file varchar,
   total_scans int,
   success_scans int,
   info varchar,
   version int,
   created timestamp default current_timestamp,
   constraint pk_processed_data primary key (file)
);


create table level0_files(
   file varchar,
   measurement_date date,
   created timestamp,
   constraint pk_level0_files_data primary key (file)
);

create table level0_files_imported(
   file varchar,
   created timestamp,
   constraint pk_level0_files_imported_data primary key (file)
);

create table level0_files_in_process(
   file varchar,
   created timestamp,
   constraint pk_level0_files_in_process_data primary key (file)
);




create table ac_level0(
   stw bigint,
   backend backend,
   frontend frontend,
   sig_type signal_type,
   ssb_att int[4],
   ssb_fq int[4],
   prescaler int,
   inttime real,
   mode int,
   acd_mon bytea,
   cc bytea, 
   file varchar,
   created timestamp default current_timestamp,
   constraint pk_ac_data primary key (backend,stw)
);
CREATE INDEX ac_level0_file_idx ON ac_level0(file, stw);

create table ac_level1a(
   stw bigint,
   backend backend,
   spectra bytea,
   created timestamp default current_timestamp,
   constraint pk_aclevel1a_data primary key (backend,stw)
);

create table ac_level1b(
   stw bigint,
   backend backend,
   frontend frontend,
   version int,
   intmode int,
   soda int,
   spectra bytea,
   channels int,
   skyfreq double precision,
   lofreq double precision,
   restfreq double precision,
   maxsuppression double precision,
   tsys real,
   sourcemode sourcemode,
   freqmode int,
   efftime real,
   sbpath real,
   calstw bigint,
   created timestamp default current_timestamp,
   constraint pk_aclevel1b_data primary key (stw,backend,frontend,version,intmode,soda,sourcemode,freqmode)
);
CREATE INDEX ac_level1b_mode_idx ON ac_level1b(backend, frontend, version, sourcemode, freqmode, intmode);


create table ac_cal_level1b(
   stw bigint,
   backend backend,
   frontend frontend,
   version int, 
   spectype spectype,
   intmode int,
   soda int,
   spectra bytea,
   channels int,
   skyfreq double precision,
   lofreq double precision,
   restfreq double precision,
   maxsuppression double precision,
   sourcemode sourcemode,
   freqmode int,
   sbpath real,
   tspill real,
   created timestamp default current_timestamp,
   constraint pk_accallevel1b_data primary key (stw,backend,frontend,version,
spectype,intmode,soda,sourcemode,freqmode)
);

create table ac_level1b_average(
   backend backend,
   frontend frontend,
   version int, 
   intmode int,
   sourcemode sourcemode,
   freqmode int,
   ssb_fq int[4],
   hotload_range real[2],
   altitude_range real[2],
   median_spectra bytea,
   mean_spectra bytea,
   channels int,
   created timestamp default current_timestamp,
   skyfreq real,
   lofreq real,
   constraint pk_aclevel1average_data primary key (backend,frontend,version,
intmode,sourcemode,freqmode, ssb_fq,hotload_range,altitude_range)
);


create table ac_cal_level1c(
   backend backend,
   frontend frontend,
   version int, 
   intmode int,
   sourcemode sourcemode,
   freqmode int,
   ssb_fq int[4],
   hotload_range real[2],
   altitude_range real[2],
   median_spectra bytea,
   median_fit bytea,
   channels int,
   created timestamp default current_timestamp,
   constraint pk_accallevel1c_data primary key (backend,frontend,version,intmode,sourcemode,freqmode, ssb_fq,hotload_range,altitude_range)
);


create table fba_level0(
   stw bigint,
   mech_type mech,
   file varchar,
   created timestamp default current_timestamp,
   constraint pk_fba_level0 primary key (stw)
);

create table attitude_level0(
   stw bigint,
   soda int,
   year int,
   mon int,
   day int,
   hour int,
   min int,
   secs double precision,
   orbit double precision,
   qt double precision[4],
   qa double precision[4],
   qe double precision[3],
   gps double precision[6],
   acs double precision,
   file varchar,
   created timestamp default current_timestamp,
   constraint pk_attitudelevel0_data primary key (stw,soda)
);

create table attitude_level1(
   stw bigint,
   backend backend,
   soda int, 
   mjd double precision,
   lst real, 
   orbit double precision,   
   latitude real,
   longitude real,
   altitude real,
   skybeamhit int,
   ra2000 real,
   dec2000 real,
   vsource real,
   qtarget double precision[4],
   qachieved double precision[4],
   qerror double precision[3],
   gpspos double precision[3],
   gpsvel double precision[3],
   sunpos double precision[3],
   moonpos double precision[3],
   sunzd real,
   vgeo real,
   vlsr real,   
   alevel int,
   created timestamp default current_timestamp,
   constraint pk_attitudelevel1_data primary key (stw,backend,soda)
);
CREATE INDEX att_level1_alt_idx ON attitude_level1(backend, altitude);
CREATE INDEX att_level1_orbit_idx ON attitude_level1(orbit, stw);

create table shk_level0(
   stw bigint,
   shk_type shk_type,
   shk_value real,
   file varchar,
   created timestamp default current_timestamp,
   constraint pk_shklevel0_data primary key (stw,shk_type)
);


create table shk_level1(
   stw bigint,
   backend backend,
   frontendsplit frontend,
   LO real,
   SSB real,
   mixC real,
   imageloadA real,
   imageloadB real,
   hotloadA real,
   hotloadB real,
   mixerA real, 
   mixerB real, 
   lnaA real,
   lnaB real,
   mixer119A real,
   mixer119B real,
   warmifA real,
   warmifB real,
   created timestamp default current_timestamp,
   constraint pk_shklevel1_data primary key (stw,backend,frontendsplit)
);
CREATE INDEX shk_level1_backend_idx ON shk_level1(backend, frontendsplit, stw);
CREATE INDEX shk_level1_hotload_idx ON shk_level1(backend, hotloada);


CREATE OR REPLACE FUNCTION public.getscansac2(stw0 bigint,stw1 bigint)
 RETURNS SETOF scan
 LANGUAGE plpgsql
AS $function$
declare
   spectrum_curs cursor for select fba_level0.stw,mech_type from fba_level0 
   join ac_level0 using(stw)
   where stw>=stw0 and stw<=stw1
   order by stw;
   res scan%rowtype;
   prev_mech fba_level0.mech_type%type;
   scanstart fba_level0.stw%type;
begin
   prev_mech:='SK1';
   scanstart:=-1;
   
   for r in spectrum_curs loop
      if r.mech_type='CAL' and prev_mech<>'CAL'  then
         scanstart:=r.stw;
      end if;
      res.start:=scanstart;
      res.stw:=r.stw;
      res.mech_type=r.mech_type;
      prev_mech:=r.mech_type; 
      return next res;
   end loop;
   return;
end;
$function$;

CREATE OR REPLACE FUNCTION public.getscansac1(stw0 bigint,stw1 bigint)
 RETURNS SETOF scan
 LANGUAGE plpgsql
AS $function$
declare
   spectrum_curs cursor for select fba_level0.stw,mech_type from fba_level0 
   join ac_level0 on (fba_level0.stw+1=ac_level0.stw)
   where ac_level0.stw>=stw0 and ac_level0.stw<=stw1
   and fba_level0.stw>=stw0-1 and fba_level0.stw<=stw1+1
   order by stw;
   res scan%rowtype;
   prev_mech fba_level0.mech_type%type;
   scanstart fba_level0.stw%type;   
begin
   prev_mech:='SK1';
   scanstart:=-2;
   
   for r in spectrum_curs loop
      if r.mech_type='CAL' and prev_mech<>'CAL' then
         scanstart:=r.stw;
      end if;
      res.start:=scanstart+1;
      res.stw:=r.stw+1;
      res.mech_type=r.mech_type;
      prev_mech:=r.mech_type; 
      return next res;
   end loop;
   return;
end;
$function$;
