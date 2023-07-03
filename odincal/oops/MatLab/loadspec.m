function [S] = loadspec(filename)
% LOADSPEC  Load a binary Odin spectrum
%    S = LOADSPEC(file) reads a binary Odin spectrum from a given file
%    on disk. It will return the spectrum as a MatLab structure.  See
%    also SAVESPEC.
  
  fid = fopen(filename, 'r');
  if fid == -1
    warning('cant open file');
    disp(filename);
  else
    S.version = sprintf('%04x', fread(fid, 1, 'ushort'));
    S.level   = sprintf('%04x', fread(fid, 1, 'ushort'));
    S.quality = sprintf('%08x', fread(fid, 1, 'ulong'));

    S.stw   = fread(fid, 1, 'ulong');
    S.mjd   = fread(fid, 1, 'double');
    S.orbit = fread(fid, 1, 'double');
    S.lst   = fread(fid, 1, 'float');

    s = fread(fid, 32, 'char');
    S.source = char(s');

    dis = ['AERO';'ASTR'];
    lookup = strvcat(' ',dis);
    S.discipline = lookup(fread(fid, 1, 'short')+1,:);

    if strcmp(S.discipline,'ASTR')
      topic = ['SOLSYS';'STARS ';'EXTGAL';'LMC   ';'PRIMOL';
	       'SPECTR';'CHEM  ';'GPLANE';'GCENTR';'GMC   ';
	       'SFORM ';'DCLOUD';'SHOCKS';'PDR   ';'HILAT ';
	       'ABSORB';'ORION ';'CALOBS';'COMMIS'         ];
    else
      topic = ['STRAT ';'ODD_N ';'ODD_H ';'WATER ';'SUMMER'; 'DYNA  ' ];
    end
    lookup = strvcat(' ', topic);
    S.topic = lookup(fread(fid, 1, 'short')+1,:);

    S.spectrum = fread(fid, 1, 'short');

    obsmode = ['TPW';'SSW';'LSW';'FSW'];
    lookup = strvcat(' ', obsmode);
    S.obsmode = lookup(fread(fid, 1, 'short')+1,:);

    type = ['SIG';'REF';'CAL';'CMB';'DRK';'SK1';'SK2';'SPE';'SSB';'AVE'];
    lookup = strvcat(' ', type);
    S.type = lookup(fread(fid, 1, 'short')+1,:);

    frontend = ['555';'495';'572';'549';'119';'SPL'];
    lookup = strvcat(' ', frontend);
    S.frontend = lookup(fread(fid, 1, 'short')+1,:);

    backend = ['AC1';'AC2';'AOS';'FBA'];
    lookup = strvcat(' ', backend);
    S.backend = lookup(fread(fid, 1, 'short')+1,:);

    S.skybeamhit = sprintf('%04x', fread(fid, 1, 'ushort'));

    S.ra2000  = fread(fid, 1, 'float');
    S.dec2000 = fread(fid, 1, 'float');
    S.vsource = fread(fid, 1, 'float');
    S.u       = fread(fid, 3, 'float')';

    S.qtarget   = fread(fid, 4, 'double')';
    S.qachieved = fread(fid, 4, 'double')';
    S.qerror    = fread(fid, 3, 'double')';

    S.gpspos = fread(fid, 3, 'double')';
    S.gpsvel = fread(fid, 3, 'double')';

    S.sunpos  = fread(fid, 3, 'double')';
    S.moonpos = fread(fid, 3, 'double')';
    S.sunzd   = fread(fid, 1, 'float');

    S.vgeo = fread(fid, 1, 'float');
    S.vlsr = fread(fid, 1, 'float');

    S.tcal = fread(fid, 1, 'float');
    S.tsys = fread(fid, 1, 'float');
    S.trec = fread(fid, 1, 'float');

    S.lofreq    = fread(fid, 1, 'double');
    S.skyfreq   = fread(fid, 1, 'double');
    S.restfreq  = fread(fid, 1, 'double');
    S.maxsup    = fread(fid, 1, 'double');
    S.freqthrow = fread(fid, 1, 'double');
    S.freqres   = fread(fid, 1, 'double');
    S.freqcal   = fread(fid, 4, 'double')';

    S.intmode = fread(fid, 1, 'int');
    S.inttime = fread(fid, 1, 'float');
    S.efftime = fread(fid, 1, 'float');

    S.channels = fread(fid, 1, 'int');
    S.data = fread(fid, Inf, 'float');
  
    fclose(fid);
  end
