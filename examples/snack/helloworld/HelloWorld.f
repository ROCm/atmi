      PROGRAM helloworld
      INCLUDE 'launch_params.f'
      
      CHARACTER(LEN=*), PARAMETER :: input = "Hello HSA World"
      CHARACTER(LEN=64) secret
      CHARACTER(LEN=64) output

C     Initialize the SIMT dimensions
      lparm%ndim=1 
      lparm%gdims(1)=LEN(input)
      lparm%ldims(1)=256
C     Call the GPU functions
      CALL encode(input,secret,lparm);
      PRINT*, "Coded message  :",secret(1:LEN(input))
      CALL decode(secret,output,lparm);
      PRINT*, "Decoded message:",output(1:LEN(input))
      END
