      PROGRAM helloworld
C     cloc -c -fort  will generate the snack_t.f file so you can set dimensions  
C     There is no reason to change the snack_t.f file. 
C     Unlike c, this file will not enforce correct types of your other arguments.
      INCLUDE 'launch_params.f'
      
      CHARACTER(LEN=*), PARAMETER :: input = "Hello HSA World"
      CHARACTER(LEN=64) secret
      CHARACTER(LEN=64) output

C     Initialize the SIMT dimensions defined in the snack_t.f file
      lparm%ndim=1 
      lparm%gdims(1)=LEN(input)
      lparm%ldims(1)=1

C     Call the GPU functions
      CALL encode(input,secret,lparm);
      PRINT*, "Coded message  :",secret(1:LEN(input))
      CALL decode(secret,output,lparm);
      PRINT*, "Decoded message:",output(1:LEN(input))
      END
