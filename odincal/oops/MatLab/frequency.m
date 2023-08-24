function [f] = frequency(S)
% FREQUENCY  return a frequency vector
%     F = FREQUENCY(S) returns the frequencies (IF) in Hz for all
%     channels in spectrum S. For the AOS F will be a vector. For the
%     correlators the frequencies are returned as a matrix with one
%     column per subband.
  
  n = S.channels;
  f = [];
  if bitand(hex2dec(S.quality), hex2dec('02000000'))
    x = [0:n-1]-floor(n/2);
    f = zeros(n,1);
    c = fliplr(S.freqcal);
    f = polyval(c, x);
    mode = 0;
  else
    if strcmp(S.backend,'AOS') == 1
      x = [0:n-1]-floor(n/2);
      c = fliplr(S.freqcal);
      f = 3900.0e6*ones(1,n)-(polyval(c, x)-2100.0e6);
      mode = 0;
    else
      if bitand(S.intmode, 256) % ADC_SEQ?
	mode = bitand(S.intmode, 255);
	bands = 0;
	seq = zeros(1,16);
	ssb = [1, -1, 1, -1, -1, 1, -1, 1];
	m = 0;
	for bit = 1:8
	  if bitget(mode,bit) 
	    m = bit;
	  end
	  seq(2*m-1) = seq(2*m-1)+1;
	end    
	for bit = 1:8
	  if seq(2*bit-1) > 0
	    seq(2*bit) = ssb(bit);
	  else
	    seq(2*bit) = 0;
	  end
	end    
	% disp(seq)

	if bitand(S.intmode, 512) % ADC_SPLIT?
	  if bitand(S.intmode, 1024) % ADC_UPPER?
	    bands = [3,4,7,8];
	  else
	    bands = [1,2,5,6];
	  end
	  f = zeros(4,112);
	else
	  bands = [1:8];
	  f = zeros(8,112);
	end
	m = 0;
	for adc = bands
	  if seq(2*adc-1) > 0
	    % [adc seq(2*adc-1)]
	    df = 1.0e6/seq(2*adc-1);
	    if seq(2*adc) < 0
	      df = -df;
	    end
	    for j=1:seq(2*adc-1)
	      % m = adc-1+j;
	      m = m+1;
	      f(m,1:112) = S.freqcal(round(adc/2))*ones(1,112)+[0:111]*df+(j-1)*112*df;
	    end
	  end
	end
      else
	df = S.freqres;
	mode = bitand(S.intmode, 15);
	if bitand(S.intmode, bitshift(1,4))
	  if bitand(S.intmode, bitshift(1,5))
	    if mode == 2
	      m = n;
	      f = S.freqcal(2)*ones(1,m)-[m-1:-1:0]*df;
	    elseif mode == 3
	      m = n/2;
	      f = [ S.freqcal(4)*ones(1,m)-[m-1:-1:0]*df;
		    S.freqcal(3)*ones(1,m)+[0:m-1]*df ];
	    else
	      m = n/4;
	      f = [ S.freqcal(3)*ones(1,m)-[m-1:-1:0]*df;
		    S.freqcal(3)*ones(1,m)+[0:m-1]*df;
		    S.freqcal(4)*ones(1,m)-[m-1:-1:0]*df;
		    S.freqcal(4)*ones(1,m)+[0:m-1]*df ];
	    end
	  else
	    if mode == 2
	      m = n;
	      f = S.freqcal(1)*ones(1,m)+[0:m-1]*df;
	      mode
	    elseif mode == 3
	      m = n/2;
	      f = [ S.freqcal(2)*ones(1,m)-[m-1:-1:0]*df;
		    S.freqcal(1)*ones(1,m)+[0:m-1]*df ];
	    else
	      m = n/4;
	      f = [ S.freqcal(1)*ones(1,m)-[m-1:-1:0]*df;
		    S.freqcal(1)*ones(1,m)+[0:m-1]*df;
		    S.freqcal(2)*ones(1,m)-[m-1:-1:0]*df;
		    S.freqcal(2)*ones(1,m)+[0:m-1]*df ];
	    end
	  end
	else
	  if mode == 1
	    m = n;
	    f = S.freqcal(1)*ones(1,m)+[0:m-1]*df;
	  elseif mode == 2
	    m = n/2;
	    f = [ S.freqcal(1)*ones(1,m)+[0:m-1]*df;
		  S.freqcal(2)*ones(1,m)-[m-1:-1:0]*df ];
	  elseif mode == 3
	    m = n/4;
	    f = [ S.freqcal(2)*ones(1,m)-[m-1:-1:0]*df;
		  S.freqcal(1)*ones(1,m)+[0:m-1]*df;
		  S.freqcal(4)*ones(1,m)-[m-1:-1:0]*df;
		  S.freqcal(3)*ones(1,m)+[0:m-1]*df ];
	  else
	    m = n/8;
	    f = [ S.freqcal(1)*ones(1,m)-[m-1:-1:0]*df;
		  S.freqcal(1)*ones(1,m)+[0:m-1]*df;
		  S.freqcal(2)*ones(1,m)-[m-1:-1:0]*df;
		  S.freqcal(2)*ones(1,m)+[0:m-1]*df;
		  S.freqcal(3)*ones(1,m)-[m-1:-1:0]*df;
		  S.freqcal(3)*ones(1,m)+[0:m-1]*df;
		  S.freqcal(4)*ones(1,m)-[m-1:-1:0]*df;
		  S.freqcal(4)*ones(1,m)+[0:m-1]*df ];
	  end
	end
      end  
    end
  end  
  if isempty(f)
    error('spectrum not frequency sorted')
  end
  f = f';
