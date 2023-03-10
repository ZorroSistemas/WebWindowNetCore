using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.DependencyInjection;
using WebWindowNetCore.Data;
using AspNetExtensions;
using Microsoft.Extensions.Logging;

namespace WebWindowNetCore;

static class Kestrel
{
    public static void Start(HttpSettings settings)
    {
        WebApplication.CreateBuilder()
            .ConfigureWebHost(webHostBuilder =>
                webHostBuilder
                    .ConfigureKestrel(options =>
                        options
                            .ListenLocalhost(settings.Port))
                    .ConfigureServices(services =>
                        services
                            .When(!string.IsNullOrEmpty(settings.CorsOrigin), s => s.AddCors())
                            .AddResponseCompression())
                    .ConfigureLogging(builder =>
                        builder
                            .AddFilter(a => a == LogLevel.Warning)
                            .AddConsole()
                            .AddDebug()))
            .Build()
            .WithResponseCompression()
            .When(!string.IsNullOrEmpty(settings.CorsOrigin), app =>
                app.WithCors(builder =>
                    builder
                        .WithOrigins("http://localhost:3000")
                        .AllowAnyHeader()
                        .AllowAnyMethod()))
            .WithRouting()
            .When(!string.IsNullOrEmpty(settings.WebrootUri), app =>
                app.WithMapSubPath(settings.WebrootUri!, async (context, subPath) =>
                {

                }))
            .Run(); 
    }
}

