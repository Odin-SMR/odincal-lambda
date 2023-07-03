function [S] = findspec(dirname, varargin)
% FINDSPEC find all Odin spectra in a given directory   
%   [S] = FINDSPEC(dir) returns all spectra in the given directory
%   (passed as a string) as an array of Odin spectrum structures. The
%   routine simply looks for all files having names like
%   'AOS.12345678.SPE', where the part up to the first dot is the name
%   of a backend like (AC1, AC2, AOS) or (A1L, A1U, A2L, A2U) for the
%   lower and upper halves of AC1 and AC2 and the file extension
%   specifies the type of spectrum
%   ('SIG','REF,',SK1','SK2','CAL','DRK','CMB','SSB','AVE')
%
%
%    [S] = FINDSPEC(dir,'backend','AOS') will return only those
%    spectra where the filename starts with 'AOS'
%
%    [S] = FINDSPEC(dir,'type','SPE') will return only those spectra
%    where the extension is 'SPE'
%
%    [S] = FINDSPEC(dir,'backend','AOS','type','SPE') will return
%    those spectra where the filename starts with 'AOS' and where the
%    extension is 'SPE'
  
  backend = 'ALL';
  type    = 'ALL';
  
  while length(varargin) > 0
    % length(varargin)
    % varargin(1)
    if strcmp('backend', varargin(1)) == 1
      backend = varargin(2);
      varargin(1:2) = [];
    elseif strcmp('type', varargin(1)) == 1
      type = varargin(2);
      varargin(1:2) = [];
    else
      varargin(1) = [];
    end
  end
  d = dir(dirname);
  n = 1;
  % dot = '.';
  % clc
  h = waitbar(0, 'Reading spectra...');
  nd = length(d);
  for i = 1:nd
    waitbar(i/nd)
    if ~d(i).isdir
      file = d(i).name;
      [smr,r] = strtok(file,'.');
      if ~isempty(smr)
	[stw,r] = strtok(r,'.');
	if ~isempty(stw)
	  % stw = hex2dec(stw);
	  [ext] = strtok(r,'.');
	end
      end
      if (length(smr) == 3) & (length(stw) == 8) & (length(ext) == 3)
	fullname = strcat(dirname,filesep,file);
	if strcmp(backend, 'ALL') | strcmp(backend, smr)
	  if strcmp(type, 'ALL') | strcmp(type, ext)
	    S(n) = loadspec(fullname);
	    n = n+1;
	  end
	end
      end
    end
  end
  close(h)
  if ~exist('S','var')
    S = [];
    msg = sprintf('no spectra found in %s', dirname);
    warning(msg);
  else
    msg = sprintf('found %d spectra in %s', length(S),dirname);
    disp(msg);
  end
