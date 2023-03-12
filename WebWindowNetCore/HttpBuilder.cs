using AspNetExtensions;
using LinqTools;
using Microsoft.AspNetCore.Http;
using WebWindowNetCore.Data;

namespace WebWindowNetCore;

public class HttpBuilder
{
    public HttpBuilder Port(int port)
        => this.SideEffect(n => Data.Port = port);
    public HttpBuilder CorsOrigin(string origin)
        => this.SideEffect(n => Data.CorsOrigin = origin);

    public HttpBuilder DefaultHtml(string defaultHtml)
        => this.SideEffect(n =>  Data.DefaultHtml = defaultHtml);

    public HttpBuilder ResourceWebroot(string resourcePath, string webrootUrl)
        => this.SideEffect(n => 
        {
            Data.ResourceWebroot = resourcePath;
            Data.WebrootUrl = webrootUrl;
        });

    public HttpBuilder UseSse<T>(string path, SseEventSource<T> sseEventSource)
        => this.SideEffect(n => 
            Data.SseDelegate = (HttpContext context) => new Sse<T>(sseEventSource.Subject).Start(context));

    public HttpBuilder UseSse<T>(string path, IObservable<T> sseEventSource)
        => this.SideEffect(n => 
            Data.SseDelegate = (HttpContext context) => new Sse<T>(sseEventSource).Start(context));

    public HttpBuilder UseJsonPost<T, TResult>(string path, Func<T, Task<TResult>> onJson)
        => this.SideEffect(n => 
            Data.JsonPostDelegate = async (HttpContext context) =>  
                {
                    var param = await context.Request.ReadFromJsonAsync<T>();
                    await context.Response.WriteAsJsonAsync<TResult>(await onJson(param!));
                });

    public HttpSettings Build() => Data.SideEffect(Kestrel.Start);

    internal HttpSettings Data { get; } = new();

    internal HttpBuilder() { }
}