function [f,d] = plotspec(S,split,vel)
% PLOTSPEC   Plot an Odin spectrum structure
%   [X,Y] = PLOTSPEC(S,N,M) will plot the channel values of spectrum S
%   against frequency or velocity. Both X and Y values of the plot
%   are are returned as vectors. 
%
%   The parameter N controls the treatment of the various correlator
%   subbands, whereas parameter M controls if a frequency or velocity
%   scale should be used on the X-axis. Frequencies will be plotted
%   (and returned) in GHz, velocities in km/s.
%
%   PLOTSPEC(S) is the same as PLOTSPEC(S,0,0)
%
%   PLOTSPEC(S,1) will show different bands of the correlator in
%   different colours and return channel values and frequencies
%   as matrices with different columns for each band. Bands are
%   clipped in order not to overlap.
%
%   PLOTSPEC(S,2) will keep frequencies where correlator bands
%   overlap.
%
%   PLOTSPEC(S,N,1) will plot channel values against velocity instead.
  
  if nargin == 1
    split = 0;
  end

  f = frequency(S);
  d = S.data;
  [m,n] = size(f);
  if n > 1
    d = reshape(S.data, m, n);
    for i = 1:n
      if f(1,i) > f(m,i)
	f(:,i) = flipud(f(:,i));
        d(:,i) = flipud(d(:,i));
      end
    end
    
    [fmin] = sort(min(f));
    [fmax,band] = sort(max(f));
    if split ~= 2
      for i = 2:n
	if fmax(i-1) > fmin(i)
	  k = fix((fmax(i-1)-fmin(i))/S.freqres/2);
	  f(m-k:m,band(i-1)) = nan;
	  d(m-k:m,band(i-1)) = nan;
	  f(1:k,band(i)) = nan;
	  d(1:k,band(i)) = nan;
	end
      end
    end
    if ~split
      f = f(:,band);
      d = d(:,band);
      f = reshape(f,prod(size(f)),1);
      d = reshape(d,prod(size(d)),1);
      i = find(isnan(f));
      f(i) = [];
      d(i) = [];
    end
  end
  % if bitand(hex2dec(S.quality), hex2dec('00001000')) == 0
  if S.skyfreq > 100.0e9
    if (S.skyfreq - S.lofreq) > 0.0
      f = S.lofreq*ones(size(f)) + f;
    else
      f = S.lofreq*ones(size(f)) - f;
    end
  end
  if nargin < 3
    vel = 0;
  end
  
  if vel == 1
    f = velocity(S);
    f = f/1000.0;
    plot(f,d), grid
    xlabel('velocity [km/s]')
  else
    f = f/1.0e9;
    f = f/(1-(S.vsource+S.vlsr)/2.997924562e8);
    plot(f,d), grid
    xlabel('frequency [GHz]')
  end
  t = sprintf('%08X  %s  %s  %s  %.2f sec', ...
	      S.stw, S.frontend, S.backend, S.type, S.inttime);
  title(t);
